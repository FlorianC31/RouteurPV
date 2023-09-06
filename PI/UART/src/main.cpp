#include <pigpio.h>
#include <cstring>
#include <iostream>

#define UART_TXD_PIN 14
#define UART_RXD_PIN 15
#define BAUD_RATE 9600

int main()
{
    float data[10];

    // Initialisation de la bibliothèque pigpio
    if (gpioInitialise() < 0)
    {
        std::cerr << "Impossible d'initialiser pigpio" << std::endl;
        return 1;
    }

    // Ouverture du port série
    int uart_handle = serOpen("/dev/serial0", BAUD_RATE, 0);
    if (uart_handle < 0)
    {
        std::cerr << "Impossible d'ouvrir le port série" << std::endl;
        gpioTerminate();
        return 1;
    }

    // Attente de la réception de données
    while (true)
    {
        if (serDataAvailable(uart_handle))
        {
            // Lecture des données reçues
            int n = 0;
            for (int i = 0; i < 10; i++)
            {
                char buf[sizeof(float)];
                bool success = true;
                for (int j = 0; j < sizeof(float); j++)
                {
                    if (serRead(uart_handle, &buf[j], 1) != 1)
                    {
                        success = false;
                        break;
                    }
                }
                if (success)
                {
                    float value;
                    std::memcpy(&value, buf, sizeof(float));
                    data[i] = value;
                    n++;
                }
                else
                {
                    std::cerr << "Erreur de lecture de données" << std::endl;
                    break;
                }
            }

            // Si toutes les données ont été lues avec succès, envoi du bit de parité
            if (n == 10)
            {
                char parity = 'p'; // Bit de parité
                if (serWrite(uart_handle, &parity, sizeof(char)) != sizeof(char))
                {
                    std::cerr << "Erreur d'envoi du bit de parité" << std::endl;
                }
            }
        }

        // Attente avant la prochaine réception de données
        time_sleep(0.1); // Attente de 100ms
    }

    // Fermeture du port série et terminaison de pigpio
    serClose(uart_handle);
    gpioTerminate();

    return 0;
}
