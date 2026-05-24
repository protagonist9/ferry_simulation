#include "../include/vehicle.h"
#include "../include/simulation.h"
#include "../include/queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* vehicle_thread(void* arg) {
    Vehicle* v = (Vehicle*)arg;
    
    // Gidiş ve dönüş için 2 döngülük bir iterasyon
    for (int trip = 0; trip < 2; trip++) { 
        
        // 1. Gişe Geçişi (Toll Booth)
        int toll_id = rand() % 2; 
        pthread_mutex_lock(&state.toll_mutex[v->current_side][toll_id]);
        
        // Gişe işlemi için rastgele gecikme
        usleep((rand() % 50 + 10) * 1000); 
        // TODO: Log toll entry
        
        pthread_mutex_unlock(&state.toll_mutex[v->current_side][toll_id]);

        // 2. Kuyruğa Girme (FIFO Queue)
        pthread_mutex_lock(&state.queue_mutex[v->current_side]);
        enqueue(&side_queues[v->current_side], v);
        // TODO: Log queue entry
        pthread_mutex_unlock(&state.queue_mutex[v->current_side]);

        // 3. Feribota Biniş (Kritik Bölge)
        pthread_mutex_lock(&state.ferry_mutex);

        // UYARI: Spurious wakeup (sahte uyanma) ihtimaline karşı her zaman while döngüsü içinde bekletiyoruz.
        // Feribot burada değilse VEYA yükleme yapmıyorsa VEYA sıradaki araç ben değilsem VEYA sığmıyorsam bekle.
        while (state.ferry_location != v->current_side || 
               !state.is_loading || 
               peek(&side_queues[v->current_side]) != v || 
               state.current_load + v->size > FERRY_CAPACITY) {
            
            // Eğer feribot buradaysa, yükleme yapıyorsa, sıra bendeyse AMA kapasite yetmiyorsa:
            // Feribotun kalkması şarttır (Departure Policy). Feribota sinyal gönder.
            if (state.ferry_location == v->current_side && state.is_loading && 
                peek(&side_queues[v->current_side]) == v && 
                state.current_load + v->size > FERRY_CAPACITY) {
                
                pthread_cond_signal(&state.cond_ferry_depart);
            }

            // Feribotun uygun duruma gelmesini bekle ve uyurken ferry_mutex'i bırak.
            pthread_cond_wait(&state.cond_ferry_available[v->current_side], &state.ferry_mutex);
        }

        // Sıra bende ve kapasite yeterli. Biniş işlemini yap.
        dequeue(&side_queues[v->current_side]);
        state.current_load += v->size;
        // TODO: Log boarding

        // Eğer feribot tam dolduysa (20 unit), kalkış sinyali gönder [cite: 53, 54]
        if (state.current_load == FERRY_CAPACITY) {
            pthread_cond_signal(&state.cond_ferry_depart);
        }

        // 4. Karşıya Geçiş ve İniş
        // Feribot karşı yakaya varıp boşaltma moduna (is_unloading) geçene kadar uyu.
        while (state.ferry_location == v->current_side || !state.is_unloading) {
            pthread_cond_wait(&state.cond_unloading_done, &state.ferry_mutex);
        }

        // Araç feribottan iniyor
        state.current_load -= v->size;
        v->current_side = 1 - v->current_side; // Yakayı değiştir (0 ise 1, 1 ise 0 yap)
        // TODO: Log unloading & arrival
        if (state.current_load == 0) {
            pthread_cond_signal(&state.cond_ferry_depart);
        }

        pthread_mutex_unlock(&state.ferry_mutex);

        // Dönüş yolculuğundan önce rastgele bir süre bekle (Return trip delay) [cite: 63, 64, 65]
        if (trip == 0) {
            usleep((rand() % 500 + 100) * 1000); // 100ms - 600ms bekleme
        }
    }

    // Gidiş-dönüş tamamlandı, aracı tamamlandı olarak işaretle [cite: 67]
    v->has_completed_trip = true;
    
    // Güvenli sayaç güncellemesi
    pthread_mutex_lock(&state.stats_mutex);
    state.completed_vehicles++;
    pthread_mutex_unlock(&state.stats_mutex);

    return NULL;
}