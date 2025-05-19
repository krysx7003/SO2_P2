# Systemy operacyjne 2

## Protokół

1. Klient wysyła własną nazwę
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

3. Żeby zacząć nową rozmowę z innym klientem należy wysłać

        \create {nazwa_własna} {nazwa_klienta1} {nazwa_klienta2} ... {nazwa_klientaN}

4. W odpowiedzi wszyscy uczestnicy otrzymują id rozmowy w formacie

        \new_conversation {id}

5. Żeby wysłać wiadomość należy wysłać (maksymalna wiadomość to 1024 znaki. Można zmienić chyba)

        \send {id_rozmowy} {tekst wiadomości}

6. Uczestnicy rozmowy do której wysłano wiadomość (poza wysyłającym) otrzymują wiadomość w formacie

        {
            "sender": "Test1",
            "text": "Testowanie strona 2",
            "timestamp": "[19.05.2025|23:36:03]"
        }

7. Żeby zakończyć połączenie należy wysłać

        \exit
