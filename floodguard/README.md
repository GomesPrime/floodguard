# 🌊 FLOOD·GUARD — Sistema IoT de Alerta de Enchentes Urbanas

> **Trabalho de Conclusão de Curso — Análise e Desenvolvimento de Sistemas**
> Universidade Presbiteriana Mackenzie (UPM) · Faculdade de Computação e Informática
> **Orientador:** Prof. Andre Luis de Oliveira
> **Autores:** João Ferreira Gomes Jr · João Victor Leiva Liceras · Raphael Campean da Silva

---

## 📋 Índice

1. [Visão Geral](#visão-geral)
2. [Demonstração em Vídeo](#demonstração-em-vídeo)
3. [Lista de Componentes](#lista-de-componentes)
4. [Instruções de Montagem](#instruções-de-montagem)
5. [Protocolo MQTT](#protocolo-mqtt)
6. [Instalação do Firmware](#instalação-do-firmware)
7. [Documentação Técnica do Software](#documentação-técnica-do-software)
8. [Dashboard Web](#dashboard-web)
9. [Estrutura do Repositório](#estrutura-do-repositório)
10. [Referências](#referências)

---

## 📌 Visão Geral

O **FLOOD·GUARD** é um sistema embarcado de baixo custo para **monitoramento e alerta de enchentes urbanas**, desenvolvido com tecnologia IoT. O dispositivo mede continuamente o nível da água, detecta precipitação e monitora as condições ambientais, classificando o risco em três níveis e transmitindo os dados em tempo real via protocolo MQTT.

### Problema que resolve
As enchentes urbanas causam impactos socioeconômicos severos em cidades brasileiras, especialmente em São Paulo. Sistemas oficiais de monitoramento (CEMADEN/ANA) ainda não cobrem completamente áreas periféricas. O FLOOD·GUARD oferece uma solução complementar de baixo custo (~R$ 229,00) acessível a comunidades vulneráveis.

### Alinhamento com ODS 11
O projeto está alinhado à **Meta 11.5** da Agenda 2030 da ONU, que visa reduzir significativamente o número de pessoas afetadas por desastres hidrológicos.

### Níveis de Alerta

| Estado | Condição | LED Semáforo | Buzzer | RGB LED |
|--------|----------|-------------|--------|---------|
| 🟢 **NORMAL** | Nível < 50% | Verde | OFF | Verde fixo |
| 🟡 **ALERTA** | 50% ≤ Nível < 80% ou chuva | Amarelo | 2 bipes | Amarelo fixo |
| 🔴 **CRÍTICO** | Nível ≥ 80% | Vermelho | Padrão SOS | Vermelho pulsante |

---

## 🎥 Demonstração em Vídeo

> **[▶ Assistir no YouTube](https://www.youtube.com/watch?v=Be38Cte1vDI)**
>
> O vídeo apresenta:
> - Funcionamento completo do hardware e sensores
> - MQTT em operação com o broker HiveMQ
> - Dashboard web recebendo dados em tempo real
> - Código-fonte explicado

---

## 🔧 Lista de Componentes

### Hardware Principal

| # | Componente | Modelo / Especificação | Qtd. | Preço Médio |
|---|-----------|----------------------|------|-------------|
| 1 | Microcontrolador | ESP32-S3-DevKitC-1 N16R8 (16MB Flash, 8MB PSRAM) | 1 | R$ 60,00 |
| 2 | Placa de Expansão (Shield) | Shield ESP32-S3 com DHT11 e slot TFT | 1 | R$ 25,00 |
| 3 | Display TFT | 2.8" ILI9341, SPI, 320×240px | 1 | R$ 46,00 |
| 4 | Sensor de Nível | HC-SR04 Ultrassônico, 40kHz, 2–400cm | 1 | R$ 8,00 |
| 5 | Sensor de Chuva | MH-RD / FR-04, AO+DO, comparador LM393 | 1 | R$ 7,00 |
| 6 | Sensor Temp/Umidade | DHT11 — embutido na shield | 1 | Incluso |
| 7 | Semáforo LED | Open-Smart Traffic Light (R/Y/G) | 1 | R$ 8,00 |
| 8 | Buzzer | Buzzer ativo 5V | 1 | R$ 5,00 |
| 9 | LED RGB | WS2812 — embutido no DevKitC-1 (GPIO 48) | 1 | Embutido |
| 10 | Bateria | 18650 Li-Ion, 3,7V, 1450–3500mAh | 1 | R$ 15,00 |
| 11 | Carregador Solar | Solar Charger v1.0 (TP4056), saída 5V | 1 | R$ 18,00 |
| 12 | Painel Solar | 45×80mm, 5V, ~90mA | 1 | R$ 12,00 |
| 13 | Cabos | Dupont Macho-Fêmea 20cm, passo 2,54mm | 20 | R$ 5,00 |
| | **TOTAL** | | | **R$ 229,00** |

### Ferramentas e Software

| Ferramenta | Versão | Finalidade |
|-----------|--------|-----------|
| Arduino IDE | 2.3.2 | Desenvolvimento e gravação do firmware |
| esp32 by Espressif | 3.x | Suporte à placa ESP32-S3 |
| Fritzing | 0.9.10 | Diagrama de montagem |
| MQTTX | 1.9.x | Cliente MQTT para testes |
| HiveMQ Broker | Cloud | Broker MQTT público |

---

## 🔌 Instruções de Montagem

### Passo 1 — Encaixe da Shield no ESP32-S3

Alinhe os pinos da shield de expansão com os headers do ESP32-S3-DevKitC-1 e pressione firmemente. Verifique que todos os pinos estão encaixados corretamente antes de prosseguir.

> ⚠️ O ESP32-S3-N16R8 tem os **GPIOs 33–37 reservados para OPI PSRAM** interna. Não use esses pinos.

### Passo 2 — Conexão do Display TFT 2.8" ILI9341

O display se encaixa diretamente no conector dedicado da shield. Nenhum cabo adicional é necessário.

| Pino TFT | GPIO ESP32-S3 | Observação |
|---------|--------------|------------|
| CS | GPIO 10 | Chip Select |
| DC/RS | GPIO 9 | Data/Command |
| RST | GPIO 8 | Reset |
| MOSI (SDI) | GPIO 11 | SPI Data |
| SCK | GPIO 14 | SPI Clock |
| VCC | 3.3V | Via shield |
| GND | GND | Via shield |

### Passo 3 — Conexão do HC-SR04 (Sensor de Nível)

> ⚠️ O HC-SR04 opera em 5V. O pino ECHO emite 5V mas o ESP32-S3 suporta apenas 3.3V. Use um **divisor de tensão** (R1=10kΩ, R2=20kΩ) no fio ECHO.

```
HC-SR04          ESP32-S3 (Shield)
VCC  ──────────  5V (terminal VIN)
GND  ──────────  GND
TRIG ──────────  GPIO 12
ECHO ──[10kΩ]── GPIO 13 ──[20kΩ]── GND
```

| Pino HC-SR04 | GPIO ESP32-S3 | Função |
|-------------|--------------|--------|
| VCC | 5V (VIN) | Alimentação |
| GND | GND | Terra |
| TRIG | GPIO 12 | Disparo (OUTPUT) |
| ECHO | GPIO 13 | Retorno (INPUT) |

### Passo 4 — Conexão do Sensor de Chuva MH-RD

```
MH-RD           ESP32-S3 (Shield)
VCC  ──────────  3.3V
GND  ──────────  GND
AO   ──────────  GPIO 7   (ADC1_CH6 — sinal analógico)
DO   ──────────  GPIO 15  (sinal digital HIGH=chuva)
```

| Pino MH-RD | GPIO ESP32-S3 | Função |
|-----------|--------------|--------|
| VCC | 3.3V | Alimentação |
| GND | GND | Terra |
| AO | GPIO 7 | Intensidade da chuva (ADC 12 bits) |
| DO | GPIO 15 | Detecção digital (HIGH = chuva) |

> 💡 Ajuste a sensibilidade girando o potenciômetro do módulo: sentido horário = maior sensibilidade.

### Passo 5 — DHT11 (já embutido na shield)

O DHT11 é embutido na shield e conectado internamente ao pino **GPIO 2** ("2G" na serigrafia). Nenhuma conexão adicional é necessária.

### Passo 6 — Semáforo LED Open-Smart

```
Semáforo        ESP32-S3
R (vermelho) ── GPIO 4   (OUTPUT — estado CRÍTICO)
Y (amarelo)  ── GPIO 5   (OUTPUT — estado ALERTA)
G (verde)    ── GPIO 6   (OUTPUT — estado NORMAL)
GND          ── GND
```

### Passo 7 — Buzzer Ativo 5V

```
Buzzer          ESP32-S3
(+)          ── GPIO 16  (OUTPUT PWM 2700Hz)
(-)          ── GND
```

> 💡 Para buzzers com corrente > 20mA, use transistor NPN BC337 entre GPIO 16 e o buzzer.

### Passo 8 — LED RGB WS2812

O LED RGB WS2812 é embutido no DevKitC-1 no **GPIO 48**. Nenhuma conexão adicional é necessária.

### Passo 9 — Sistema de Alimentação Solar

```
Painel Solar (5V)
  (+) ──► SOLAR+ do TP4056
  (-) ──► SOLAR- do TP4056

Bateria 18650
  (+) ──► BAT+  do TP4056
  (-) ──► BAT-  do TP4056

Saída do TP4056
  OUT+ ──► Interruptor ON/OFF ──► VIN da Shield
  OUT- ──► GND da Shield
```

### Resumo Completo de Pinos GPIO

| GPIO | Pino / Shield | Componente | Direção | Protocolo |
|------|--------------|-----------|---------|-----------|
| GPIO 2 | "2G" shield | DHT11 — Temperatura/Umidade | INPUT | 1-Wire |
| GPIO 4 | Header externo | LED Vermelho — Semáforo | OUTPUT | Digital |
| GPIO 5 | Header externo | LED Amarelo — Semáforo | OUTPUT | Digital |
| GPIO 6 | Header externo | LED Verde — Semáforo | OUTPUT | Digital |
| GPIO 7 | Header externo | MH-RD — Sinal Analógico (AO) | INPUT | ADC 12 bits |
| GPIO 8 | Conector TFT | ILI9341 — Reset | OUTPUT | SPI |
| GPIO 9 | Conector TFT | ILI9341 — Data/Command | OUTPUT | SPI |
| GPIO 10 | Conector TFT | ILI9341 — Chip Select | OUTPUT | SPI |
| GPIO 11 | Conector TFT | ILI9341 — MOSI | OUTPUT | SPI |
| GPIO 12 | Terminal parafuso | HC-SR04 — Trigger | OUTPUT | Digital |
| GPIO 13 | Terminal parafuso | HC-SR04 — Echo | INPUT | pulseIn() |
| GPIO 14 | Conector TFT | ILI9341 — SCK | OUTPUT | SPI |
| GPIO 15 | Header externo | MH-RD — Sinal Digital (DO) | INPUT | Digital |
| GPIO 16 | Header externo | Buzzer Ativo 5V | OUTPUT | PWM/LEDC 2700Hz |
| GPIO 48 | Embutido placa | LED RGB WS2812 | OUTPUT | NeoPixel 1-Wire |

---

## 📡 Protocolo MQTT

### O que é MQTT

O **MQTT (Message Queuing Telemetry Transport)** é um protocolo de comunicação leve baseado no modelo **publish/subscribe**, projetado para dispositivos IoT com recursos limitados. Desenvolvido pela IBM e padronizado pela OASIS (2014), opera sobre TCP/IP com overhead mínimo de 2 bytes de cabeçalho.

```
┌─────────────┐    PUBLISH     ┌──────────────┐    SUBSCRIBE   ┌────────────────┐
│  ESP32-S3   │ ─────────────► │   BROKER     │ ─────────────► │  Dashboard Web │
│  FLOOD·GUARD│                │   HiveMQ     │                │  MQTTX / App   │
└─────────────┘                └──────────────┘                └────────────────┘
   Sensores coletam              Gerencia e                       Recebe dados
   dados a cada 5s               roteia mensagens                 em tempo real
```

### Configuração do Broker

| Parâmetro | Valor |
|-----------|-------|
| Broker Host | `broker.hivemq.com` |
| Porta TCP | `1883` |
| Porta WebSocket | `8000` |
| Porta TLS/SSL | `8883` |
| Autenticação | Nenhuma (broker público) |
| Client ID | `floodguard_N16R8_001` |

### Tópicos MQTT

| Tópico | Direção | Descrição | QoS | Retain |
|--------|---------|-----------|-----|--------|
| `floodguard/upm/sensor1/dados` | ESP32 → Broker | Payload JSON com todos os dados dos sensores | 0 | false |
| `floodguard/upm/sensor1/cmd` | Broker → ESP32 | Comandos remotos (reset, calibrar) | 0 | false |
| `floodguard/upm/sensor1/status` | ESP32 → Broker (LWT) | Status: `"online"` / `"offline"` | 0 | true |

### Estrutura do Payload JSON

Publicado a cada **5 segundos** no tópico `floodguard/upm/sensor1/dados`:

```json
{
  "dispositivo": "floodguard_N16R8_001",
  "nivel_pct":   42,
  "dist_cm":     58.0,
  "chuva":       false,
  "rain_a0":     120,
  "temp_c":      26.0,
  "umidade":     65.0,
  "alerta":      "NORMAL",
  "uptime_s":    3600,
  "rssi":        -62,
  "rgb_mode":    "MQTT_OK"
}
```

| Campo | Tipo | Descrição | Exemplo |
|-------|------|-----------|---------|
| `dispositivo` | string | ID único do dispositivo | `"floodguard_N16R8_001"` |
| `nivel_pct` | integer | Nível da água em % (0–100) | `42` |
| `dist_cm` | float | Distância HC-SR04 → superfície (cm) | `58.0` |
| `chuva` | boolean | Detecção digital MH-RD (true = chuva) | `false` |
| `rain_a0` | integer | Leitura ADC MH-RD (0–4095) | `120` |
| `temp_c` | float | Temperatura DHT11 (°C) | `26.0` |
| `umidade` | float | Umidade relativa DHT11 (%) | `65.0` |
| `alerta` | string | Estado: NORMAL \| ALERTA \| CRITICO | `"NORMAL"` |
| `uptime_s` | integer | Tempo de operação (segundos) | `3600` |
| `rssi` | integer | Sinal Wi-Fi (dBm) | `-62` |
| `rgb_mode` | string | Estado do LED RGB WS2812 | `"MQTT_OK"` |

### Como se conectar ao broker (MQTTX)

1. Baixe o **MQTTX** em [mqttx.app](https://mqttx.app)
2. Crie uma nova conexão:
   - **Host:** `broker.hivemq.com`
   - **Porta:** `1883`
   - **Client ID:** `mqttx_monitor_001` (qualquer nome único)
   - **Username/Password:** deixar em branco
3. Clique em **Connect**
4. Adicione uma subscription: `floodguard/upm/sensor1/#`
5. Ligue o ESP32 — dados chegam a cada 5 segundos

### Comando remoto (reset)

Publique no tópico `floodguard/upm/sensor1/cmd`:
```json
{"cmd": "reset"}
```

### LED RGB — Indicador de Status MQTT

| Cor | Comportamento | Significado |
|-----|--------------|-------------|
| ⬜ Branco | Pulsante | Boot / Inicializando |
| 🩵 Ciano | Pulsante | Tentando conectar ao Wi-Fi |
| 🩵 Ciano | Fixo | Wi-Fi OK, MQTT offline |
| 🔵 Azul | Fixo | Wi-Fi + MQTT conectados ✅ |
| 🔵 Azul | Flash 150ms | Publicando dado MQTT |
| 🟡 Amarelo | Intermitente | Wi-Fi perdido |
| 🟣 Magenta | Fixo | Sem Wi-Fi (modo offline) |
| 🔴 Vermelho | Pulsante | Nível CRÍTICO |

---

## 💻 Instalação do Firmware

### Pré-requisitos

1. **Arduino IDE 2.3.2** — [arduino.cc/en/software](https://www.arduino.cc/en/software)
2. **Pacote ESP32 by Espressif** (versão 3.x)
3. Bibliotecas (instalar via Library Manager):

```
Adafruit GFX Library        >= 1.11.9
Adafruit ILI9341            >= 1.6.0
DHT sensor library          >= 1.4.6
Adafruit Unified Sensor     >= 1.1.14
Adafruit NeoPixel           >= 1.12.0
PubSubClient                >= 2.8.0
ArduinoJson                 >= 6.21.5
```

### Configuração da Placa no Arduino IDE

| Parâmetro | Valor |
|-----------|-------|
| Board | `ESP32S3 Dev Module` |
| Upload Speed | `921600` |
| CPU Frequency | `240 MHz (WiFi)` |
| Flash Size | `16MB (128Mb)` |
| Partition Scheme | `Huge APP (3MB No OTA/1MB SPIFFS)` |
| PSRAM | **`OPI PSRAM`** ← obrigatório para N16R8 |
| USB Mode | `Hardware CDC and JTAG` |

### Configuração de Rede

Abra o arquivo `FloodGuard_N16R8_v2_2.ino` e edite as linhas:

```cpp
const char* WIFI_SSID     = "SEU_WIFI";       // Nome da rede Wi-Fi
const char* WIFI_PASSWORD = "SUA_SENHA";      // Senha da rede Wi-Fi
```

### Calibração do Sensor HC-SR04

Ajuste conforme o local de instalação:

```cpp
#define DIST_MAXIMA    100  // Distância sensor → fundo VAZIO (cm)
#define NIVEL_ALERTA    50  // % para entrar em ALERTA
#define NIVEL_CRITICO   80  // % para entrar em CRÍTICO
```

### Gravação

1. Conecte o ESP32-S3 via USB-C
2. Selecione a porta COM correta
3. Pressione **Upload (Ctrl+U)**
4. Após gravação, abra o **Monitor Serial** (115200 baud) para verificar a conexão

---

## 📊 Documentação Técnica do Software

### Arquitetura do Firmware

O firmware opera em **ciclo não bloqueante** usando `millis()` para múltiplos temporizadores:

```
loop() a cada iteração
├── 500ms  → readSensors() → updateAlertLevel() → updateLEDs()
├── 600ms  → buzzerUpdate() (máquina de padrão sonoro)
├── 1000ms → drawCurrentPage() (atualiza TFT)
├── 5000ms → mqttLoop() (publica JSON no broker)
└── ∞      → updateRgbLed() (atualiza LED RGB)
```

### Lógica de Classificação de Risco

```
Distância HC-SR04 (Dm) + Distância fundo (Df = 100cm)
         │
         ▼
N(%) = ((Df - Dm) / Df) × 100
         │
         ├── N < 50%         → NORMAL   → LED Verde,  sem buzzer
         ├── 50% ≤ N < 80%   → ALERTA   → LED Amarelo, 2 bipes
         │   OU chuva = true
         └── N ≥ 80%         → CRÍTICO  → LED Vermelho, SOS buzzer
```

### Cálculo de Distância HC-SR04

```
Pulso TRIG: 10µs HIGH → burst 8 pulsos 40kHz → mede duração ECHO

d (cm) = (duração_echo_µs × 0.0343) / 2

Timeout: 25ms (protege contra leitura inválida)
```

### Padrões do Buzzer (PWM LEDC)

```cpp
ledcAttach(BUZZER_PIN, 2700, 8);  // 2700 Hz, 8 bits resolução
ledcWrite(BUZZER_PIN, 200);        // duty 78% = máximo volume

// ALERTA:  [120ms ON][80ms OFF][120ms ON] ──── 1000ms ────
// CRÍTICO: [80ms][60ms][80ms][60ms][80ms][120ms][250ms][100ms][250ms] ─ 500ms ─
```

### Páginas do Dashboard TFT (rotação automática 8s)

| Página | Conteúdo |
|--------|----------|
| HOME | Tanque animado, nível%, chuva, temperatura, umidade, histórico 60s |
| SENSORES | Barras de progresso individuais + RSSI Wi-Fi |
| ALERTA | Semáforo visual, estado atual, células de dados |
| MQTT/SISTEMA | Status conexão, broker, tópico, publicações, uptime |

### Bibliotecas e Dependências

| Biblioteca | Versão | Uso |
|-----------|--------|-----|
| Adafruit GFX | 1.11.9 | Primitivas gráficas TFT |
| Adafruit ILI9341 | 1.6.0 | Driver display SPI |
| DHT sensor library | 1.4.6 | Leitura DHT11/DHT22 |
| Adafruit Unified Sensor | 1.1.14 | Dependência DHT |
| Adafruit NeoPixel | 1.12.x | LED RGB WS2812 GPIO 48 |
| PubSubClient | 2.8.0 | Cliente MQTT |
| ArduinoJson | 6.21.5 | Serialização JSON |

---

## 🌐 Dashboard Web

O arquivo `FloodGuard_Dashboard.html` é um dashboard standalone que:
- Conecta ao broker HiveMQ via **WebSocket (porta 8000)**
- Exibe dados em tempo real sem servidor
- Funciona offline para testes (botão 🧪 TESTAR)
- Suporta tema **claro/escuro** com persistência

### Como usar

1. Abra `FloodGuard_Dashboard.html` no Chrome/Firefox
2. Preencha: `broker.hivemq.com` | porta `8000` | tópico `floodguard/upm/sensor1/#`
3. Clique **CONECTAR**
4. Ligue o ESP32 — dados aparecem automaticamente

---

## 📁 Estrutura do Repositório

```
floodguard
│
├── artigo
│   └── Artigo_FloodGuard.pdf
│
├── codigo
│   └── floodguard.ino
│
├── dashboard
│   ├── index.html
│
├── fritzing
│   └── diagrama.png
│   └── FloodGuard.fzz           
│
├── imagens
│   ├── prototipo.jpg
│   ├── mqttx.jpg
│   └── dashboard.jpg
│
└── README.md
---

## 📚 Referências

- ATZORI, L.; IERA, A.; MORABITO, G. The Internet of Things: A survey. *Computer Networks*, v. 54, n. 15, 2010.
- GUBBI, J. et al. Internet of Things (IoT): A vision, architectural elements, and future directions. *Future Generation Computer Systems*, v. 29, n. 7, 2013.
- ESPRESSIF SYSTEMS. ESP32-S3 Series Datasheet. Disponível em: https://www.espressif.com
- MQTT.ORG. MQTT: The Standard for IoT Messaging. Disponível em: http://mqtt.org
- OASIS. MQTT Version 3.1.1. OASIS Standard, 2014.
- HIVEMQ. MQTT Essentials. Disponível em: https://www.hivemq.com/mqtt-essentials/
- FRITZING. Electronics made easy. Disponível em: https://fritzing.org

---

## 👥 Equipe

| Nome | RA | E-mail |
|------|----|--------|
| João Ferreira Gomes Jr | 10369051 | 10369051@mackenzista.com.br |
| João Victor Leiva Liceras | 10290101 | 10290101@mackenzista.com.br |
| Raphael Campean da Silva | 10424088 | 10424088@mackenzista.com.br |

**Orientador:** Prof. Andre Luis de Oliveira
**Instituição:** Universidade Presbiteriana Mackenzie (UPM) — São Paulo, 2026

---

*Este projeto foi desenvolvido como Trabalho de Conclusão de Curso do curso de Análise e Desenvolvimento de Sistemas da Universidade Presbiteriana Mackenzie, alinhado ao ODS 11 da Agenda 2030 da ONU.*
