
1. Rozpakować archiwum np do C:\ddeproxy
2. Uruchomić wiersz poleceń z prawami administratora:
Start->Uruchom:
runas /user:localhost\administrator cmd

3. W cmd:
cd c:\ddeproxy
ddeproxy.exe -install

(to zainstaluje proxy jako usługę windows).

4. Start->Panel sterownia->Narzedzia administracyjne->Uslugi
(uwaga ta sciezka do pod xp dla 2000 moze byc inna), prawym na DDE
proxy service, i w zakładce Logowanie proszę zaznaczyć
'Zezwalaj na współdzialanie z pulpitem' oraz ustawić żeby usługa
startowała automatycznie.

5. Aby zmiana ustawień z pkt 4 została uwzględniona należy
zatrzymać usługę a następnie uruchomić ją ponownie.

Usługa standardowo nasłuchuje połączeń na porcie 8080. Jeśli komputer
ma uruchomioną zaporę albo zainstalowanego jakiegoś firewalla
proszę odblokować połączenia przychodzące na ten port. 

