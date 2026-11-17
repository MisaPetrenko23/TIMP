# TIMP

MFC-приложение (C++/Visual Studio) для шифрования по Цезарю с подключением к базе **RedDatabase / Firebird**.

## Готовый запуск

Готовая программа лежит в `bin/MFCApplication1.exe` (Release x64).  
Просто запустите её — база `MFCApplication1/CAESARCIPHERDB.FDB` будет найдена автоматически.

Для работы нужен установленный клиент RedDatabase/Firebird (`fbclient.dll`, обычно в `C:\Windows\system32`).

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

> Файл `CAESARCIPHERDB.FDB` теперь лежит в репозитории: `MFCApplication1/CAESARCIPHERDB.FDB`.
> При сборке он автоматически копируется в папку с exe (post-build event), поэтому вручную ничего копировать не нужно.
