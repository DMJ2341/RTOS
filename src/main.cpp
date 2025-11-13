#include <Arduino.h>
#include <STM32FreeRTOS.h>   // incluye FreeRTOS para SAM (Due)

/* ---------- Constantes ---------- */
constexpr int THRESHOLD_ON  = 28;   // °C
constexpr int THRESHOLD_OFF = 26;

/* ---------- Cola FreeRTOS ---------- */
struct SensorData {
    int temperature;   // °C
    uint32_t us;       // micros() para medir latencia
};
QueueHandle_t xTempQueue;

/* ---------- Salidas "virtuales" ---------- */
static void led(bool on) { Serial.print(on ? "[LED ON] " : "[LED OFF] "); }
static void fan(bool on) { Serial.print(on ? "[FAN ON] " : "[FAN OFF] "); }
static void uart(const char *msg) { Serial.print("[UART] "); Serial.println(msg); }

/* ---------- Tareas ---------- */
static void vReaderTask(void *pv) {
    SensorData data;
    for (;;) {
        data.temperature = random(20, 36);          // 20-35 °C
        data.us          = micros();
        xQueueSend(xTempQueue, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(250));             // 4 Hz
    }
}

static void vLogicTask(void *pv) {
    SensorData rx;
    enum { NORMAL, ALERT } state = NORMAL;
    for (;;) {
        xQueueReceive(xTempQueue, &rx, portMAX_DELAY);
        uint32_t latency = micros() - rx.us;        // tiempo de respuesta

        if (state == NORMAL && rx.temperature > THRESHOLD_ON) {
            state = ALERT;
            led(1); fan(1);
            uart("ALERTA: " + String(rx.temperature) + " °C");
        } else if (state == ALERT && rx.temperature <= THRESHOLD_OFF) {
            state = NORMAL;
            led(0); fan(0);
            uart("NORMAL: " + String(rx.temperature) + " °C");
        }

        if (latency > 0) {
            Serial.print("[TIMING] ");
            Serial.print(latency);
            Serial.println(" µs");
        }
    }
}

/* ---------- Setup ---------- */
void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("=== LAB EMBEDDED SIMULADO ===");
    Serial.println("FreeRTOS + Arduino Due");
    Serial.println("");

    xTempQueue = xQueueCreate(4, sizeof(SensorData));
    configASSERT(xTempQueue);

    xTaskCreate(vReaderTask, "Reader", 256, NULL, 1, NULL);
    xTaskCreate(vLogicTask,  "Logic",  256, NULL, 2, NULL);

    vTaskStartScheduler();
}

/* ---------- Loop vacío (RTOS controla) ---------- */
void loop() { }
