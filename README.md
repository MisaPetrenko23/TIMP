# TIMP

MFC-приложение (C++/Visual Studio) для шифрования по Цезарю с подключением к базе **RedDatabase / Firebird**.

## Подключение к БД

Используется файл `CAESARCIPHERDB.FDB`. Программа ищет его:

1. в папке с `MFCApplication1.exe`;
2. в родительской папке;
3. в `MFCApplication1\x64\Debug`.

Параметры подключения (в `MFCApplication1Dlg.cpp`, метод `ConnectDatabase`):

- пользователь: `SYSDBA`
- пароль: `masterkey`
- кодировка соединения: `UTF8`

При запуске в окне отображается статус подключения. Операции `Шифровать`, `Дешифровать` и `Взломать` записываются в таблицы:

- `SOURCEFILES` — имя, размер и MD5-хэш исходного файла;
- `ENCRYPTIONOPERATIONS` — тип операции, сдвиг, дата;
- `CRACKANALYSIS` — найденный сдвиг, метрика частотного анализа и длина текста (только для взлома).

Первый пользователь из таблицы `USERS` (`Петренко М.Р.`) используется как автор операций.

## Сборка

В Visual Studio 2022 (v143, x64) откройте `MFCApplication1.sln`.

В `MFCApplication1.vcxproj` уже настроены:

- `C:\Program Files\RedDatabase\include`
- `C:\Program Files\RedDatabase\lib`
- библиотека `fbclient_ms.lib`

Во время выполнения используется `fbclient.dll` из RedDatabase (установлена в `C:\Windows\system32`).

> Файл БД не хранется в GitHub. Положите `CAESARCIPHERDB.FDB` рядом с exe перед запуском.
