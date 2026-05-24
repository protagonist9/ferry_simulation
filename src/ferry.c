#include "../include/ferry.h"
#include "../include/simulation.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

// Maksimum bekleme süresi (milisaniye) - Starvation önlemi
#define FERRY_MAX_WAIT_MS 3000 

void* ferry_thread(void* arg) {
    while (1) {
        // 1. Sonlandırma Kontrolü
        pthread_mutex_lock(&state.stats_mutex);
        if (state.completed_vehicles == TOTAL_VEHICLES) { // Tüm araçlar gidiş-dönüş yaptı [cite: 69-70]
            pthread_mutex_unlock(&state.stats_mutex);
            break; 
        }
        pthread_mutex_unlock(&state.stats_mutex);

        // 2. YÜKLEME DURUMU (LOADING)
        pthread_mutex_lock(&state.ferry_mutex);
        state.is_unloading = false;
        state.is_loading = true;

        // O yakada bekleyen tüm araçlara "feribot geldi ve yüklemeye hazır" sinyali gönder
        pthread_cond_broadcast(&state.cond_ferry_available[state.ferry_location]);

        // Timeout (Zaman Aşımı) Hesaplaması
        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, NULL);
        
        long wait_ms = now.tv_usec / 1000 + FERRY_MAX_WAIT_MS;
        timeout.tv_sec = now.tv_sec + (wait_ms / 1000);
        timeout.tv_nsec = (wait_ms % 1000) * 1000000;

        // Feribot kalkış sinyali bekler VEYA süre dolana kadar uyur
        // Araçlar doluluk veya sığmama durumunda cond_ferry_depart sinyali atacak.
        pthread_cond_timedwait(&state.cond_ferry_depart, &state.ferry_mutex, &timeout);

        // Kalkış işlemi başlıyor
        state.is_loading = false;
        // TODO: Log departure [cite: 111]
        
        pthread_mutex_unlock(&state.ferry_mutex);

        // 3. SEYAHAT (TRAVEL)
        usleep((rand() % 200 + 300) * 1000); // 300-500 ms arası seyahat süresi

        // 4. İNİŞ DURUMU (UNLOADING)
        pthread_mutex_lock(&state.ferry_mutex);
        
        // Yakayı değiştir
        state.ferry_location = 1 - state.ferry_location;
        // TODO: Log arrival [cite: 103]

        state.is_unloading = true;
        
        // Karşıya geçmeyi bekleyen (uyuyan) araçları uyandır [cite: 60]
        pthread_cond_broadcast(&state.cond_unloading_done);

        // Tüm araçlar inene kadar bekle. Busy waiting yasak olduğu için [cite: 81] burada uyu.
        while (state.current_load > 0) {
            pthread_cond_wait(&state.cond_ferry_depart, &state.ferry_mutex); 
        }
        
        pthread_mutex_unlock(&state.ferry_mutex);
    }

    return NULL;
}