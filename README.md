# Systemy operacyjne 2

## Serwer czatu

### Opis

Celem projektu było stworzenie prostego serewera do czatowania. Serwer miał być lokalny ( uruchomiony na localhoscie ) tak samo klienci.

### Założenia

1. Nie znamy liczby klientów którzy będą chcieli się połączyć
2. Klienci będą chcieli tworzyć "prywatne" rozmowy między sobą
3. Rozmowy mogą być między wieloma klientami (rozmowy grupowe)
4. Jeżeli odbiorca nie jest aktywny wiadomość należy przechować i dostarczyć gdy ponownie się zaloguje
5. Klienci chcą widzieć historię obecnej rozmowy

### Wątki i sekcje krytyczne

Po stronie serwera jest dynamiczna pula wątków które odpowiadają liczbie obecnie połączonych klientów(dla każdego klienta osobny wątek). Na wątku główny obsługiwane są początkowe etapy połączenia z klientem. Następnie twożony jest nowy wątek który obsługuje dalszą komunikację.

Sekcje krytyczne to dodawanie klienta(obiektu), i wątku do puli oraz wysyłanie danych do klienta.

Po stronie klienta są dwa wątki główny (obsługuje input użytkownika), oraz drugi wątek który obsługuje odbieranie wiadomości i wypisywanie ich w konsoli.

### Protokół połączenia

1. Klient wysyła własną nazwę w formacie

        {
            "type": "register",
            "sender": {nazwa podana przez użytkownika}
        }

2. Klient odbiera pliki .json z prowadzonymi rozmowami (np. build/data/0.json)

        {
            "id": 0,
            "messages_log": [
                {
                    "sender": "Test",
                    "text": "Testowanie",
                    "timestamp": "[19.05.2025|23:35:17]"
                },
                {
                    "sender": "Test1",
                    "text": "Testowanie strona 2",
                    "timestamp": "[19.05.2025|23:36:03]"
                }
            ],
            "users": [
                "Test",
                "Test1"
            ]
        }

3. Żeby zacząć nową rozmowę z innym klientem należy wpisać (z okna klienta)

        \create {nazwa_własna} {nazwa_odbiorcy_1} {nazwa_odbiorcy_2} 

    Następnie program wysyła:

        {
            "type": "command",
            "command": "\\create",
            "id": {id_rozmowy},
            "users": {lista_odbiorców}
        }

4. W odpowiedzi wszyscy uczestnicy otrzymują rozmowę w formacie

        {
            "id": 0,
            "messages_log": [
                {
                    "sender": "Test",
                    "text": "Testowanie",
                    "timestamp": "[19.05.2025|23:35:17]"
                },
                {
                    "sender": "Test1",
                    "text": "Testowanie strona 2",
                    "timestamp": "[19.05.2025|23:36:03]"
                }
            ],
            "users": [
                "Test",
                "Test1"
            ]
        }

5. Żeby wysłać wiadomość należy wysłać z okna klienta (maksymalna wiadomość to 1024 znaki. Można zmienić chyba)

        {id_rozmowy} {wiadomość}

    Następnie program wysyła:

        {
            "type": "message",
            "command": "\\send",
            "id": {id_rozmowy},
            "message": {wiadomość}
        }

6. Uczestnicy rozmowy do której wysłano wiadomość (poza wysyłającym) otrzymują wiadomość w formacie

        {
            "sender": "Test1",
            "text": "Testowanie strona 2",
            "timestamp": "[19.05.2025|23:36:03]"
        }

7. Żeby zakończyć połączenie należy wysłać

        \exit

    Następnie program wysyła:

        {
            "type": "message",
            "command": "\\exit"
        }

### Źródła

1. <https://www.geeksforgeeks.org/dining-philosopher-problem-using-semaphores/>
2. <https://lass.cs.umass.edu/~shenoy/courses/spring04/677/readings/Chandy_drinking_phil.pdf>
3. <https://enomem.substack.com/p/the-drinking-philosophers-problem>
4. <https://www.geeksforgeeks.org/dining-philosophers-problem/>
5. <https://www.youtube.com/watch?v=-Qa1RqmXKG0>
6. <https://www.youtube.com/watch?v=Zp17-UDKM90>
7. <https://www.youtube.com/watch?v=hXKtYRleQd8>
8. <https://www.youtube.com/watch?v=FMNnusHqjpw&t>
9. <https://www.youtube.com/watch?v=Pg_4Jz8ZIH4&t>
10. <https://www.youtube.com/watch?v=P6Z5K8zmEmc&t>
