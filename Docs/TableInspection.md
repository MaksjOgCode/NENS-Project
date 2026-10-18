# Осмотр стола и записка

`BP_Table` открывает общий `WBP_InspectionScene` через прежний `Event Interact`. На столе лежит одна записка: клик записывает сбор и скрывает предмет, Escape закрывает осмотр. Если выйти без сбора, стол можно открыть снова. После сбора и выхода E и кольцо скрываются и больше не появляются при пересечении зон. Сам стол остаётся видимым.

## Где менять сцену

Открой `/Game/NENS/UI/Inspection/DA_Inspection_Table`:

| Поле | Значение |
| --- | --- |
| Background | `/Game/NENS/UI/Inspection/Textures/T_Inspection_Table` |
| Canvas Size | `2001 × 1561`, исходный размер фона |
| Pickups[0].Pickup ID | `Table_Note`, конкретное размещение |
| Pickups[0].Item ID | `Note`, тип собранного предмета |
| Pickups[0].Image | `/Game/NENS/UI/Inspection/Textures/T_Note` |
| Pickups[0].Position | `540, 850`, верхний левый угол записки на фоне |
| Pickups[0].Size | `328, 384.5`, с сохранением пропорций PNG |

Чтобы переместить или изменить размер записки, меняй только Position/Size в этом Data Asset. Общий виджет масштабирует фон и предметы совместно. Новые предметы добавляются в Pickups с уникальным Pickup ID. Размеры и позиции задаются в пикселях исходного фона.

## Где находится подключение

`/Game/NENS/Environment/Interior/Blueprints/BP_Table`:

- Components → **Searchable**: Scene = `DA_Inspection_Table`, Scene Widget Class = `WBP_InspectionScene`.
- EventGraph → **Event Interact** → `Open Inspection(Self, Interactor)`.
- EventGraph → **On Inspection Closed (Searchable)** → Branch Completed. При true вызываются `SetAwarenessVisible(false)` и `SetHiddenInGame(true)` для `Widget_InteractionPrompt` и `Ring_Visual`.

Графы можно исправлять в Blueprint. Движение, ввод, Escape и подтверждённый сбор обслуживает существующий общий C++ механизм, описанный в [InspectionSystem.md](InspectionSystem.md). Отдельный виджет стола не создаётся.

Прогресс независим у каждого экземпляра стола и хранится до конца запуска игры. Полноценного инвентаря и SaveGame нет. Для будущего инвентаря используется уже существующее событие Searchable → `On Item Collected(PickupID, ItemID)`; для этой сцены Item ID равен `Note`.

## Автоматическая настройка

`Tools/configure_table_inspection.py` импортирует только новые фон и записку и подключает существующий стол через редакторский `ConfigureSearchableActor`. Повторный запуск использует существующие текстуры и данные, не сбрасывая положение записки. Обычное редактирование сцены не требует запуска скрипта. Исходные PNG, спрайт мебели, карта и общий WBP при настройке не меняются.

Перед первым запуском настройки создана копия `Saved/TableInspection/backup-before-table`. Скрипт предназначен для Unreal Python commandlet; редактор должен быть закрыт, чтобы два процесса не записывали одни ассеты.

## Проверки

`NENS.Inspection.Integration.PIETable` проверяет вход через существующий BPC, фон и пропорции записки, курсор, блокировку движения, Escape после клика на фон и записку, выход без сбора и повторный вход, настоящий клик по записке, запрет повторного сбора, сохранение окна до Escape, отключение завершённого стола, отсутствие мерцания при пересечениях зон и независимый второй экземпляр стола.

Отчёты задачи находятся в `Saved/TableInspection`, логи — в `Saved/Logs/TableInspection*.log`. Физическую клавишу E и удобство расположения записки на своём размере окна можно проверить вручную; привязка E не менялась.

Проверка 18 октября 2026: сборка Editor успешна, BP_Table компилируется с 0 ошибок и 0 предупреждений. Все 7 тестов `NENS.` прошли, включая стол, шкаф и провода. В первом тесте зарегистрировано постороннее предупреждение LogHttp о таймауте проверки подключения к сети; ошибок тестов и `Accessed None`/`RingMID` нет. Скриншоты `table-open.png` и `table-collected.png` находятся в `Saved/TableInspection`, отчёт — `Saved/TableInspection/Automation/index.json`. Хеши подтвердили изменение только BP_Table среди существующих Content-файлов; добавлены два изображения и ресурс данных сцены. Runtime-код не менялся, полная упаковка с cook в этой задаче не запускалась.
