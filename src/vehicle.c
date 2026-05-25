#include "../include/vehicle.h"
#include "../include/simulation.h"
#include "../include/queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/logger.h"

void* vehicle_thread(void* arg) {
    Vehicle* v = (Vehicle*)arg;
    
    // Gidiş ve dönüş için 2 döngülük bir iterasyon
    for (int trip = 0; trip < 2; trip++) { 
        
        // 1. Gişe Geçişi (Toll Booth)
        int toll_id = rand() % 2; 
        pthread_mutex_lock(&state.toll_mutex[v->current_side][toll_id]);
        usleep((rand() % 50 + 10) * 1000); 
        log_vehicle_event(v, v->current_side == SIDE_A ? "entered toll on Side A" : "entered toll on Side B");
        pthread_mutex_unlock(&state.toll_mutex[v->current_side][toll_id]);

        // 2. Kuyruğa Girme (FIFO Queue)
        pthread_mutex_lock(&state.queue_mutex[v->current_side]);
        enqueue(&side_queues[v->current_side], v);
        log_vehicle_event(v, v->current_side == SIDE_A ? "joined queue on Side A" : "joined queue on Side B");
        long wait_start = get_current_time_ms(); 
        pthread_mutex_unlock(&state.queue_mutex[v->current_side]);

        // EKSTRA MANTIKSAL İYİLEŞTİRME (Two-Side Coordination):
        // Ben karşı yakadaysam ve feribot boş bir şekilde diğer yakada bekliyorsa onu doğrudan buraya çağır.
        pthread_mutex_lock(&state.ferry_mutex);
        if (state.ferry_location != v->current_side && state.is_loading && state.current_load == 0) {
            pthread_cond_signal(&state.cond_ferry_depart);
        }
        pthread_mutex_unlock(&state.ferry_mutex);

        // 3. Feribota Biniş (Kritik Bölge)
        pthread_mutex_lock(&state.ferry_mutex);

        while (1) {
            // HATA 1 ÇÖZÜMÜ: Kuyruktaki aracı kontrol ederken queue_mutex kilitlenmeli
            pthread_mutex_lock(&state.queue_mutex[v->current_side]);
            Vehicle* front_vehicle = peek(&side_queues[v->current_side]);
            pthread_mutex_unlock(&state.queue_mutex[v->current_side]);

            // Biniş şartları sağlandıysa döngüden çık
            if (state.ferry_location == v->current_side && 
                state.is_loading && 
                front_vehicle == v && 
                state.current_load + v->size <= FERRY_CAPACITY) {
                break; 
            }

            // Kalkış Politikası: Sıra bende ama kapasite yetmiyorsa, feribotun kalkması şarttır.
            if (state.ferry_location == v->current_side && state.is_loading && 
                front_vehicle == v && 
                state.current_load + v->size > FERRY_CAPACITY) {
                pthread_cond_signal(&state.cond_ferry_depart);
            }

            // Feribotun uygun duruma gelmesini bekle
            pthread_cond_wait(&state.cond_ferry_available[v->current_side], &state.ferry_mutex);
        }

        // HATA 1 ÇÖZÜMÜ: Kuyruktan çıkış yaparken queue_mutex kilitlenmeli
        pthread_mutex_lock(&state.queue_mutex[v->current_side]);
        dequeue(&side_queues[v->current_side]);
        pthread_mutex_unlock(&state.queue_mutex[v->current_side]);
        
        state.current_load += v->size;
        
        long wait_time = get_current_time_ms() - wait_start;
        record_wait_time(wait_time);
        log_vehicle_event(v, "boarded the ferry");

        // HATA 2 ÇÖZÜMÜ: Binen araç arkasındaki diğer aracın şansını denemesi için sinyal göndermeli (Cascade Wakeup)
        pthread_cond_broadcast(&state.cond_ferry_available[v->current_side]);

        // Eğer feribot tam dolduysa (20 unit), kalkış sinyali gönder
        if (state.current_load == FERRY_CAPACITY) {
            pthread_cond_signal(&state.cond_ferry_depart);
        }

        // 4. Karşıya Geçiş ve İniş
        while (state.ferry_location == v->current_side || !state.is_unloading) {
            pthread_cond_wait(&state.cond_unloading_done, &state.ferry_mutex);
        }

        // Araç feribottan iniyor
        state.current_load -= v->size;
        v->current_side = 1 - v->current_side; // Yakayı değiştir 
        log_vehicle_event(v, "unloaded from the ferry");
        
        // Eğer feribotu tamamen ben boşalttıysam, feribotu uyandır.
        if (state.current_load == 0) {
            pthread_cond_signal(&state.cond_ferry_depart);
        }

        pthread_mutex_unlock(&state.ferry_mutex);

        // Dönüş yolculuğundan önce rastgele bir süre bekle
        if (trip == 0) {
            usleep((rand() % 500 + 100) * 1000); 
        }
    }

    // Gidiş-dönüş tamamlandı, aracı tamamlandı olarak işaretle
    v->has_completed_trip = true;
    
    // Güvenli sayaç güncellemesi
    pthread_mutex_lock(&state.stats_mutex);
    state.completed_vehicles++;
    pthread_mutex_unlock(&state.stats_mutex);

    return NULL;
}
