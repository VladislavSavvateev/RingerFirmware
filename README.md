# RingerFirmware
Прошивка Звонка, написанная для ESP8266 с использованием фреймворка Arduino.
### Что такое Звонок?
Звонок - это аппаратно-программный комплекс, который позволяет замыкать реле в строго заданное время. Почему Звонок? Да потому что обычно подобное лучше всего подходит в школе, где звонки подаются по расписанию и нужно замыкать цепь со звонками.
### Компоненты Звонка
+ WeMos D1 с ESP8266 на борту
+ обычное реле на 10 ампер 220 вольт (D4)
+ OLED-дисплей 0.96 дюймов 128х64 (D3, D5; I2C)
+ DS1307 модуль (D3, D5; I2C)
### Принцип работы
Всё очень просто. В начале работы Звонок проводит инициализацию компонентов:
+ дисплей
+ EEPROM
+ Wi-Fi (о нём несколько позже)

После чего из EEPROM Звонок получает следующий звонок. Каждый раз после того, как было перемкнуто реле на время или наступил следующий день, Звонок достаёт из EEPROM новый звонок.
##### Структура хранения данных в EEPROM:
+ структура дня (повторяется семь раз)
  + 1 байт на количество звонков (N)
  + структура звонка (повторяется N раз)
    + 1 байт на показатель того, включен ли звонок
    + 1 байт на час
    + 1 байт на минуту
    
### Для чего нужен Wi-Fi?
После сборки всего Звонка нужно было придумать способ загружать расписание в EEPROM. Подключаться к Звонку по UART каждый раз было бы странно и крайне неудобно для конечного пользователя, поэтому было принято решение создать TCP-слушатель соединений. Звонок при начальной инициализации создаёт точку доступа Wi-Fi, где от каждого её пользователя ждёт TCP-сокет по порту 1488 (порт был выбран случайным образом). Через этот сокет Звонок получает команды и данные для хранения.
Входной фрейм состоит из байта, который определяет команду, и дальнейших байтов, которые являются входными данными.
Выходной фрейм состоит из результирующего байта (всегда 1) и (если есть) дальнейших байтов, которые являются выходными данными. 
##### Список команд
+ #1 - вернуть текущее расписание
  + Выход:
    + структура выходных данных совпадает со структурой хранения в EEPROM
+ #2 - очистить память
+ #3 - замкнуть реле
+ #4 - разомкнуть реле
+ #5 - установить текущее время
  + Вход:
    + 1 байт на час
    + 1 байт на минуту
    + 1 байт на секунду
    + 1 байт на день
    + 1 байт на месяц
    + 1 байт на год
+ #6 - установить текущее расписание 
  + Вход:
    + структура входных данных совпадает со структурой хранения в EEPROM
### Приложение для настройки
Как я уже упомянул ранее, Звонок ждёт TCP-сокет. Для его открытия было решено создать [приложение для Android](https://github.com/VladislavSavvateev/RingerSetup), которое имело бы функционал для работы с поддерживаемыми командами. 
