# Guía de Instalación y Configuración del Broker Mosquitto en PC

Esta guía detalla los pasos para convertir tu PC en el **Broker MQTT (Mosquitto)** al que se conectarán el **ESP32** (Publicador) y el **Celular** (Suscriptor).

---

## 1. Configuración de la Red Wi-Fi (Punto de Acceso / Router)

Para que el ESP32, la PC y el Celular se comuniquen mediante MQTT, **deben estar en la misma red local**.

### Opción Recomendada: Zona con Cobertura Inalámbrica (Hotspot desde la PC)
1. En tu PC (Windows o Linux), activa la opción **"Zona con cobertura inalámbrica móvil" / "Mobile Hotspot"**.
2. Asigna un nombre (SSID) y contraseña a la red Wi-Fi.
3. Conecta el **ESP32** y tu **Celular** a esta red Wi-Fi generada por la PC.
4. Obtén la IP de tu PC en la red:
   - **En Windows**: Abre la CMD y ejecuta `ipconfig`. Busca la IP en "Adaptador de LAN inalámbrica".
   - **En Linux**: Abre la terminal y ejecuta `hostname -I` o `ip a`.

---

## 2. Instalación y Ejecución de Mosquitto

### En Windows
1. Descarga e instala **Mosquitto** desde la web oficial ([mosquitto.org/download/](https://mosquitto.org/download/)).
2. Navega a la carpeta de instalación (generalmente `C:\Program Files\mosquitto`).
3. Copia el archivo `mosquitto.conf` creado en este proyecto dentro de esa carpeta.
4. Abre la Terminal/CMD como **Administrador** y ejecuta:
   ```cmd
   mosquitto.exe -c mosquitto.conf -v
   ```
5. **Configuración del Firewall de Windows**:
   - Abre "Firewall de Windows con seguridad avanzada".
   - Agrega una nueva **Regla de Entrada**: Puerto TCP `1883`, Permitir la conexión.

### En CachyOS / Arch Linux
1. **Instalar Mosquitto**:
   CachyOS utiliza el gestor de paquetes `pacman` (o `yay` / `paru` si utilizas AUR). Instala el broker ejecutando:
   ```bash
   sudo pacman -S mosquitto
   ```
2. **Ejecutar el Broker (Modo Depuración / Recomendado para la práctica)**:
   Para ver en tiempo real los mensajes que entran y salen en tu consola, ejecuta Mosquitto cargando la configuración del proyecto:
   ```bash
   mosquitto -c /home/moka/Disco/Robo2/mosquitto_config/mosquitto.conf -v
   ```
3. **Ejecutar como Servicio de Sistema (Segundo plano)**:
   Si prefieres dejarlo corriendo en segundo plano:
   - Copia el archivo de configuración a la ubicación por defecto:
     ```bash
     sudo cp /home/moka/Disco/Robo2/mosquitto_config/mosquitto.conf /etc/mosquitto/mosquitto.conf
     ```
   - Inicia y habilita el servicio:
     ```bash
     sudo systemctl start mosquitto
     sudo systemctl enable mosquitto
     ```

### En Linux (Ubuntu / Debian)
1. Instala mosquitto y mosquitto-clients:
   ```bash
   sudo apt update
   sudo apt install -y mosquitto mosquitto-clients
   ```
2. Ejecuta el broker usando el archivo de configuración del proyecto:
   ```bash
   sudo mosquitto -c /home/moka/Disco/Robo2/mosquitto_config/mosquitto.conf -v
   ```

### Cortafuegos (Firewall) en Linux
Si tienes un cortafuegos activo en CachyOS/Arch, abre el puerto `1883`:
- **Si usas UFW**:
  ```bash
  sudo ufw allow 1883/tcp
  ```
- **Si usas Firewalld**:
  ```bash
  sudo firewall-cmd --add-port=1883/tcp --permanent
  sudo firewall-cmd --reload
  ```

---

## 3. Verificación Rápida del Broker desde la PC

Puedes abrir dos terminales en la PC para probar que el broker funciona:

- **Terminal 1 (Suscriptor PC)**:
  ```bash
  mosquitto_sub -h 127.0.0.1 -t "robotica/estacion1/#" -v
  ```

- **Terminal 2 (Publicador de prueba)**:
  ```bash
  mosquitto_pub -h 127.0.0.1 -t "robotica/estacion1/telemetria/temperatura" -m "45.5"
  ```

Si ves el mensaje `"robotica/estacion1/telemetria/temperatura 45.5"` en la Terminal 1, ¡tu broker está funcionando perfectamente!
