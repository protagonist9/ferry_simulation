#ifndef SIMULATION_H
#define SIMULATION_H

#include <pthread.h>
#include <stdbool.h>

// Sistem Parametreleri (Proje Dokümanından)
#define CAR_SIZE 1         // [cite: 25]
#define MINIBUS_SIZE 2     // [cite: 26]
#define TRUCK_SIZE 3       // [cite: 27]
#define FERRY_CAPACITY 20  // [cite: 29]

#define NUM_CARS 12        // [cite: 25]
#define NUM_MINIBUSES 10   // [cite: 26]
#define NUM_TRUCKS 8       // [cite: 27]
#define TOTAL_VEHICLES (NUM_CARS + NUM_MINIBUSES + NUM_TRUCKS)

// Yakalar [cite: 13, 14]
typedef enum {
    SIDE_A = 0,
    SIDE_B = 1
} Side;

// Araç Tipleri
typedef enum {
    CAR = 0,
    MINIBUS,
    TRUCK
} VehicleType;

// Araç Veri Yapısı
typedef struct {
    int id;
    VehicleType type;
    int size;
    Side origin_side;    // [cite: 17, 20]
    Side current_side;
    bool has_completed_trip;
} Vehicle;

// Sistemin Global Durumu
typedef struct {
    // Feribotun Mevcut Durumu
    int current_load;
    Side ferry_location;
    bool is_loading;
    bool is_unloading;

    // Gişe Kilitleri: Her yaka için 2 gişe [cite: 41, 75]
    pthread_mutex_t toll_mutex[2][2]; 

    // Kuyruk Kilitleri: Her yaka için 1 kuyruk kilidi [cite: 77]
    // Kuyruğa ekleme/çıkarma işlemleri sırasında kullanılacak
    pthread_mutex_t queue_mutex[2];   

    // Feribot Ana Kilidi (Yükleme/Boşaltma koruması) [cite: 76]
    pthread_mutex_t ferry_mutex;      

    // Sinyalleşme (Condition Variables) 
    // 0: A Yakası, 1: B Yakası için
    pthread_cond_t cond_ferry_available[2]; // Araçlar feribota binmek için bu sinyali bekler
    pthread_cond_t cond_ferry_depart;       // Feribot kalkış şartını (dolma/süre) bekler [cite: 53-56]
    pthread_cond_t cond_unloading_done;     // Karşıya geçildiğinde araçlar inmek için bu sinyali bekler [cite: 60]
    
    // İstatistikler için değişkenler (Loglama modülünde kullanacağız)
    int completed_vehicles;
    pthread_mutex_t stats_mutex;
} SystemState;

// Tüm .c dosyalarından erişilebilecek global state referansı
extern SystemState state;

#endif