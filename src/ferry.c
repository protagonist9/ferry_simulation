#include "../include/ferry.h"
#include "../include/simulation.h"
#include "../include/queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "../include/logger.h"

// Maksimum bekleme süresi (milisaniye) - Starvation önlemi
#define FERRY_MAX_WAIT_MS 3000 

void* ferry_thread(void* arg) {
    while (1) {
        // 1. Sonlandırma Kontrolü
        pthread_mutex_lock(&state.stats_mutex);
        if (state.completed_vehicles == TOTAL_VEHICLES) { // Tüm araçlar gidiş-dönüş yaptı
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

        // HATA 3 ÇÖZÜMÜ: İki Yaka Koordinasyonu (Two-Side Coordination)
        pthread_mutex_lock(&state.queue_mutex[state.ferry_location]);
        int current_side_count = side_queues[state.ferry_location].size;
        pthread_mutex_unlock(&state.queue_mutex[state.ferry_location]);

        pthread_mutex_lock(&state.queue_mutex[1 - state.ferry_location]);
        int opposite_side_count = side_queues[1 - state.ferry_location].size;
        pthread_mutex_unlock(&state.queue_mutex[1 - state.ferry_location]);

        // Bulunduğu yakada araç yoksa ve karşı yakada bekleyen araç varsa hiç beklemeden kalk.
        if (current_side_count == 0 && opposite_side_count > 0 && state.current_load == 0) {
            // Timeout beklemeyi pas geç, doğrudan kalkışa izin ver.
        } else {
            // Timeout (Zaman Aşımı) Hesaplaması
            struct timeval now;
            struct timespec timeout;
            gettimeofday(&now, NULL);
            
            long wait_ms = now.tv_usec / 1000 + FERRY_MAX_WAIT_MS;
            timeout.tv_sec = now.tv_sec + (wait_ms / 1000);
            timeout.tv_nsec = (wait_ms % 1000) * 1000000;

            pthread_cond_timedwait(&state.cond_ferry_depart, &state.ferry_mutex, &timeout);
        }

        // Kalkış işlemi başlıyor
        state.is_loading = false;
        record_ferry_trip(state.current_load);
        log_ferry_event(state.ferry_location == SIDE_A ? "departed from Side A" : "departed from Side B", state.current_load);
        
        pthread_mutex_unlock(&state.ferry_mutex);

        // 3. SEYAHAT (TRAVEL)
        usleep((rand() % 200 + 300) * 1000); // 300-500 ms arası seyahat süresi

        // 4. İNİŞ DURUMU (UNLOADING)
        pthread_mutex_lock(&state.ferry_mutex);
        
        // Yakayı değiştir
        state.ferry_location = 1 - state.ferry_location;
        log_ferry_event(state.ferry_location == SIDE_A ? "arrived at Side A" : "arrived at Side B", state.current_load);

        state.is_unloading = true;
        
        // Karşıya geçmeyi bekleyen (uyuyan) araçları uyandır 
        pthread_cond_broadcast(&state.cond_unloading_done);

        // Tüm araçlar inene kadar bekle. 
        while (state.current_load > 0) {
            pthread_cond_wait(&state.cond_ferry_depart, &state.ferry_mutex); 
        }
        
        pthread_mutex_unlock(&state.ferry_mutex);
    }

    return NULL;
}
