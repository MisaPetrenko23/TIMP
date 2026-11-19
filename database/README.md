# База данных TIMP

Файл: `MFCApplication1/CAESARCIPHERDB.FDB` (копия для готовой сборки лежит в `bin/CAESARCIPHERDB.FDB`).

СУБД: **RedDatabase / Firebird 3**, диалект 3, кодировка `UTF8`.

## Структура

| Таблица | Назначение | Записей |
|---|---|---:|
| `USERS` | Пользователи: `ID`, `USERNAME`, `PASSWORDHASH` (SHA-256, 64 HEX), `AVATAR` (BLOB), `CREATEDAT` | 11 |
| `SOURCEFILES` | Загруженные файлы: `ID`, `USERID` (FK → `USERS`), `FILENAME`, `FILESIZE`, `FILEHASH` (MD5, 32 HEX) | 15 |
| `ENCRYPTIONOPERATIONS` | Операции: `ID`, `FILEID` (FK → `SOURCEFILES`), `OPERATIONTYPE`, `SHIFTKEY`, `STATUS`, `OPERATIONDATE` | 25 |
| `CRACKANALYSIS` | Результаты взлома: `ID`, `OPERATIONID` (FK → `ENCRYPTIONOPERATIONS`), `FOUNDSHIFT`, `FITNESSSCORE`, `TEXTLENGTH` | 10 |

**Итого: 61 запись.**

Связи «один-ко-многим» реализованы внешними ключами. Первичные ключи заполняются триггерами `BEFORE INSERT` из генераторов `GEN_USERS_ID`, `GEN_FILES_ID`, `GEN_OPERATIONS_ID`, `GEN_ANALYSIS_ID`.

## Воссоздание базы

Из корня репозитория (удалите старый `.fdb`, если он есть):

```powershell
$env:FIREBIRD = (Resolve-Path 'bin\firebird').Path
& 'C:\Program Files\RedDatabase\isql.exe' -q /database\create_database.sql
```

Скрипт создаст `MFCApplication1\CAESARCIPHERDB.FDB`, схему, триггеры и тестовые данные.

## Проверка

```powershell
$sql = @'
SELECT (SELECT COUNT(*) FROM USERS) AS USERS,
       (SELECT COUNT(*) FROM SOURCEFILES) AS FILES,
       (SELECT COUNT(*) FROM ENCRYPTIONOPERATIONS) AS OPS,
       (SELECT COUNT(*) FROM CRACKANALYSIS) AS ANALYSIS
FROM RDB$DATABASE;
'@
$sql | & 'C:\Program Files\RedDatabase\isql.exe' -user SYSDBA -password masterkey 'MFCApplication1\CAESARCIPHERDB.FDB'
```

Ожидаемый результат: `11`, `15`, `25`, `10`.
