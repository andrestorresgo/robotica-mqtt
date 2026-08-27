#!/usr/bin/env bash
# =============================================================================
# GREENHOUSE MQTT QUICK SETUP & LIVE DEMO LAUNCHER (ARCH LINUX)
# =============================================================================

set -e

# ANSI Color Codes
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_FILE="${SCRIPT_DIR}/mosquitto_config/mosquitto.conf"

# -----------------------------------------------------------------------------
# 1. Dependency Checks & Auto-Install
# -----------------------------------------------------------------------------
check_dependencies() {
    echo -e "${CYAN}==> Checking system dependencies...${NC}"
    
    if ! command -v mosquitto &> /dev/null || ! command -v mosquitto_sub &> /dev/null; then
        echo -e "${YELLOW}[!] Mosquitto is not installed.${NC}"
        echo -e "Installing Mosquitto via pacman..."
        sudo pacman -S --needed mosquitto
    else
        echo -e "${GREEN}[✓] Mosquitto is installed ($(mosquitto -h | head -n 1)).${NC}"
    fi
}

# -----------------------------------------------------------------------------
# 2. Network IP Detection
# -----------------------------------------------------------------------------
get_local_ip() {
    # Try wlan0 first, then default route
    WLAN_IP=$(ip -4 addr show wlan0 2>/dev/null | grep -oP '(?<=inet\s)\d+(\.\d+){3}' || true)
    if [ -n "$WLAN_IP" ]; then
        echo "$WLAN_IP"
    else
        hostname -I | awk '{print $1}'
    fi
}

print_header() {
    clear
    LOCAL_IP=$(get_local_ip)
    echo -e "${BLUE}${BOLD}====================================================================${NC}"
    echo -e "${CYAN}${BOLD}    🌿 GREENHOUSE CLIMATE CONTROL - MQTT QUICK DEMO LAUNCHER 🌿    ${NC}"
    echo -e "${BLUE}${BOLD}====================================================================${NC}"
    echo -e "  ${BOLD}Host IP Address (Wi-Fi):${NC}  ${GREEN}${BOLD}${LOCAL_IP}${NC}"
    echo -e "  ${BOLD}MQTT Port:${NC}                ${GREEN}1883${NC} (Unencrypted / Anonymous)"
    echo -e "  ${BOLD}Config File:${NC}              ${CYAN}${CONFIG_FILE}${NC}"
    echo -e "  ${BOLD}Root Topic Tree:${NC}          ${MAGENTA}greenhouse/#${NC}"
    echo -e "${BLUE}====================================================================${NC}"
    echo ""
}

# -----------------------------------------------------------------------------
# Option 1: Start Mosquitto Broker in Foreground
# -----------------------------------------------------------------------------
start_broker() {
    print_header
    echo -e "${YELLOW}==> Starting Mosquitto Broker in foreground...${NC}"
    echo -e "${CYAN}Press [Ctrl + C] anytime to stop the broker.${NC}\n"
    mosquitto -c "$CONFIG_FILE" -v
}

# -----------------------------------------------------------------------------
# Option 2: Live Telemetry Subscriber
# -----------------------------------------------------------------------------
live_monitor() {
    print_header
    echo -e "${YELLOW}==> Connecting Live Subscriber to greenhouse/# ...${NC}"
    echo -e "${CYAN}Listening for live sensor readings, button events, and servo movements...${NC}"
    echo -e "${CYAN}Press [Ctrl + C] to exit monitor.${NC}\n"
    
    mosquitto_sub -h localhost -p 1883 -t "greenhouse/#" -v | while read -r line; do
        TIMESTAMP=$(date +"%H:%M:%S")
        TOPIC=$(echo "$line" | awk '{print $1}')
        PAYLOAD=$(echo "$line" | cut -d' ' -f2-)
        
        case "$TOPIC" in
            "greenhouse/status")
                echo -e "[$TIMESTAMP] ${BOLD}STATUS:${NC}      ${GREEN}$PAYLOAD${NC}" ;;
            "greenhouse/temperature")
                echo -e "[$TIMESTAMP] ${BOLD}TEMP:${NC}        ${YELLOW}${PAYLOAD} °C${NC}" ;;
            "greenhouse/humidity")
                echo -e "[$TIMESTAMP] ${BOLD}HUMIDITY:${NC}    ${CYAN}${PAYLOAD} %${NC}" ;;
            "greenhouse/potentiometer")
                echo -e "[$TIMESTAMP] ${BOLD}POT DIAL:${NC}    ${MAGENTA}${PAYLOAD} %${NC}" ;;
            "greenhouse/motion")
                if [ "$PAYLOAD" == "MOTION_DETECTED" ]; then
                    echo -e "[$TIMESTAMP] ${BOLD}PIR MOTION:${NC}  ${RED}${BOLD}⚠ MOTION DETECTED ⚠${NC}"
                else
                    echo -e "[$TIMESTAMP] ${BOLD}PIR MOTION:${NC}  ${GREEN}CLEAR (Idle)${NC}"
                fi ;;
            "greenhouse/servo1/status"|"greenhouse/servo2/status")
                echo -e "[$TIMESTAMP] ${BOLD}ACTUATOR:${NC}    ${BLUE}$TOPIC -> $PAYLOAD${NC}" ;;
            "greenhouse/button1"|"greenhouse/button2")
                echo -e "[$TIMESTAMP] ${BOLD}BUTTON:${NC}      ${YELLOW}$TOPIC -> $PAYLOAD${NC}" ;;
            "greenhouse/mode/status")
                echo -e "[$TIMESTAMP] ${BOLD}MODE:${NC}        ${BOLD}$PAYLOAD${NC}" ;;
            *)
                echo -e "[$TIMESTAMP] $TOPIC : $PAYLOAD" ;;
        esac
    done
}

# -----------------------------------------------------------------------------
# Option 3: Interactive Actuator & Mode Controller
# -----------------------------------------------------------------------------
interactive_control() {
    while true; do
        print_header
        echo -e "${BOLD}Select a command to publish to the ESP32:${NC}"
        echo -e "  ${CYAN}[1]${NC} Open Roof Vent (Servo 1)          ${MAGENTA}-> greenhouse/servo1/set OPEN${NC}"
        echo -e "  ${CYAN}[2]${NC} Close Roof Vent (Servo 1)         ${MAGENTA}-> greenhouse/servo1/set CLOSE${NC}"
        echo -e "  ${CYAN}[3]${NC} Open Side Vent (Servo 2)          ${MAGENTA}-> greenhouse/servo2/set OPEN${NC}"
        echo -e "  ${CYAN}[4]${NC} Close Side Vent (Servo 2)         ${MAGENTA}-> greenhouse/servo2/set CLOSE${NC}"
        echo -e "  ${CYAN}[5]${NC} Open Both Vents (45° Half-Open)   ${MAGENTA}-> Angle 45 on both servos${NC}"
        echo -e "  ${CYAN}[6]${NC} Set Custom Angle (0 - 180°)       ${MAGENTA}-> Prompt for angle${NC}"
        echo -e "  ${CYAN}[7]${NC} Set Mode to AUTOMATIC             ${MAGENTA}-> greenhouse/mode/set AUTO${NC}"
        echo -e "  ${CYAN}[8]${NC} Set Mode to MANUAL                ${MAGENTA}-> greenhouse/mode/set MANUAL${NC}"
        echo -e "  ${RED}[0] Return to Main Menu${NC}"
        echo ""
        read -rp "Enter option [0-8]: " choice

        case "$choice" in
            1)
                mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo1/set" -m "OPEN"
                echo -e "${GREEN}[✓] Published OPEN to greenhouse/servo1/set${NC}"
                sleep 1 ;;
            2)
                mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo1/set" -m "CLOSE"
                echo -e "${GREEN}[✓] Published CLOSE to greenhouse/servo1/set${NC}"
                sleep 1 ;;
            3)
                mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo2/set" -m "OPEN"
                echo -e "${GREEN}[✓] Published OPEN to greenhouse/servo2/set${NC}"
                sleep 1 ;;
            4)
                mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo2/set" -m "CLOSE"
                echo -e "${GREEN}[✓] Published CLOSE to greenhouse/servo2/set${NC}"
                sleep 1 ;;
            5)
                mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo1/set" -m "45"
                mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo2/set" -m "45"
                echo -e "${GREEN}[✓] Published 45° to both vents${NC}"
                sleep 1 ;;
            6)
                read -rp "Enter target angle (0 to 180): " custom_angle
                if [[ "$custom_angle" =~ ^[0-9]+$ ]] && [ "$custom_angle" -ge 0 ] && [ "$custom_angle" -le 180 ]; then
                    mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo1/set" -m "$custom_angle"
                    mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo2/set" -m "$custom_angle"
                    echo -e "${GREEN}[✓] Set both vents to ${custom_angle}°${NC}"
                else
                    echo -e "${RED}[!] Invalid angle. Must be 0 - 180.${NC}"
                fi
                sleep 1.5 ;;
            7)
                mosquitto_pub -h localhost -p 1883 -t "greenhouse/mode/set" -m "AUTO"
                echo -e "${GREEN}[✓] Switched system to AUTO mode${NC}"
                sleep 1 ;;
            8)
                mosquitto_pub -h localhost -p 1883 -t "greenhouse/mode/set" -m "MANUAL"
                echo -e "${GREEN}[✓] Switched system to MANUAL mode${NC}"
                sleep 1 ;;
            0)
                break ;;
            *)
                echo -e "${RED}[!] Invalid option.${NC}"
                sleep 1 ;;
        esac
    done
}

# -----------------------------------------------------------------------------
# Option 4: Automated 20-Second Presentation Demo Routine
# -----------------------------------------------------------------------------
run_automated_demo() {
    print_header
    echo -e "${MAGENTA}${BOLD}==> STARTING AUTOMATED GREENHOUSE PRESENTATION DEMO <==${NC}\n"
    
    echo -e "${CYAN}[Step 1/5] Switching to MANUAL mode...${NC}"
    mosquitto_pub -h localhost -p 1883 -t "greenhouse/mode/set" -m "MANUAL"
    sleep 2

    echo -e "${CYAN}[Step 2/5] Opening Roof Vent (Servo 1 -> 90°)...${NC}"
    mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo1/set" -m "OPEN"
    sleep 3

    echo -e "${CYAN}[Step 3/5] Opening Side Vent (Servo 2 -> 90°)...${NC}"
    mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo2/set" -m "OPEN"
    sleep 3

    echo -e "${CYAN}[Step 4/5] Moving both vents to 45° Dehumidification position...${NC}"
    mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo1/set" -m "45"
    mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo2/set" -m "45"
    sleep 3

    echo -e "${CYAN}[Step 5/5] Closing all vents and restoring AUTO climate control...${NC}"
    mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo1/set" -m "CLOSE"
    mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo2/set" -m "CLOSE"
    mosquitto_pub -h localhost -p 1883 -t "greenhouse/mode/set" -m "AUTO"
    sleep 1

    echo -e "\n${GREEN}${BOLD}[✓] Automated presentation routine completed successfully!${NC}"
    echo -e "Press Enter to return to menu..."
    read -r
}

# -----------------------------------------------------------------------------
# Option 5: Launch Python Terminal Dashboard
# -----------------------------------------------------------------------------
run_python_subscriber() {
    print_header
    if command -v python3 &> /dev/null; then
        python3 "${SCRIPT_DIR}/test_scripts/mqtt_pc_subscriber.py"
    else
        echo -e "${RED}[!] python3 not found.${NC}"
        sleep 2
    fi
}

# -----------------------------------------------------------------------------
# Main Interactive Menu Loop
# -----------------------------------------------------------------------------
main() {
    check_dependencies
    
    while true; do
        print_header
        echo -e "${BOLD}Select an action:${NC}"
        echo -e "  ${GREEN}[1] Start Mosquitto Broker${NC} (Foreground with live logs)"
        echo -e "  ${CYAN}[2] Live Telemetry Monitor${NC} (Colorized stream of sensors & vents)"
        echo -e "  ${YELLOW}[3] Interactive Actuator & Mode Controller${NC} (Remote control menu)"
        echo -e "  ${MAGENTA}[4] Run Automated 20s Presentation Routine${NC} (Test all servos & modes)"
        echo -e "  ${BLUE}[5] Run Python Terminal Subscriber${NC}"
        echo -e "  ${RED}[0] Exit${NC}"
        echo ""
        read -rp "Enter choice [0-5]: " main_choice

        case "$main_choice" in
            1) start_broker ;;
            2) live_monitor ;;
            3) interactive_control ;;
            4) run_automated_demo ;;
            5) run_python_subscriber ;;
            0) 
                echo -e "\n${GREEN}Exiting. Happy demoing! 🌿${NC}\n"
                exit 0 ;;
            *) 
                echo -e "${RED}[!] Invalid choice.${NC}"
                sleep 1 ;;
        esac
    done
}

main "$@"
