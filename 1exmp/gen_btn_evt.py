import RPi.GPIO as GPIO
import time

PIN = 20

def setup():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(PIN, GPIO.OUT)

def toggle_pin():
    GPIO.output(PIN, GPIO.HIGH)
    time.sleep(0.2)
    GPIO.output(PIN, GPIO.LOW)

def cleanup():
    GPIO.cleanup()


def main():
    setup()
    toggle_pin()
    cleanup()

if __name__ == '__main__':
    main()
    