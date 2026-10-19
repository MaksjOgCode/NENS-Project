# Проверка вычислений

`BP_ComputerCabinet` теперь использует спрайт `T_Furniture_Computer` и открывает проверку вычислений. Подойти в белое кольцо → **E**. Четыре кадра проявляются по очереди; после последнего появляется поле ответа. **29** завершает проверку, ошибочный ответ показывает **Incorrect result. Please try again.** Можно сразу исправить число и нажать **Enter** или **Check**. **Esc / Exit** плавно закрывают сцену; незавершённая попытка при повторном входе начинается с первого кадра.

Результат записывается как **Arithmetic check — READY** в главном компьютере. Для восстановления теперь требуются Power, Fuel, Display, Arithmetic и обе загруженные перфокарты. Проверки можно проходить в любом порядке. После успеха показывается короткое подтверждение, окно плавно закрывается, подсказка предмета убирается. Прогресс хранится до конца текущего запуска игры, как у остальных загадок.

## Что ставить на уровень

| Blueprint | Назначение | Папка |
| --- | --- | --- |
| BP_ComputerCabinet | Новая математическая проверка | `/Game/NENS/Environment/Interior/Blueprints` |
| BP_Props_ElectroStation | Провода, питание компьютера | `/Game/NENS/Environment/Interior/Props/Blueprints` |
| BP_Props_DataComputer | Главный терминал восстановления | `/Game/NENS/Environment/Interior/Props/Blueprints` |

Все три доступны в общей коллекции **NENS_Demo_Props_Ready** Content Browser. Электростанция использует существующий `SPR_Props_Electro_Station`, прежнюю логику проводов и стандартные кольцо / E. Старые размещения `BP_ComputerCabinet` автоматически открывают вычисления. На `L_MainRooms` декоративные электростанция и главный компьютер заменены интерактивными объектами; остальные размещения сохраняются.

## Настройки

- `/Game/NENS/UI/Arithmetic/DA_ArithmeticComputer`: **Frames** — четыре изображения по порядку, **Answer** — 29, **Hold Seconds** — 0.85, **Blend Seconds** — 0.65, **Input Fade Seconds** — 0.35.
- `/Game/NENS/UI/Arithmetic/WBP_ArithmeticComputer`: Designer — макет; Class Defaults → Puzzle — данные; общие Transition / Mist / Appearance — появление, закрытие, дымка и шрифт IM Fell English.
- `BP_ComputerCabinet → Minigame`: Widget Class = WBP_ArithmeticComputer, Repaired System = Arithmetic.
- `BP_Props_ElectroStation → Minigame`: Widget Class = WBP_WiresMiniGame, Repaired System = Power.

Исходные PNG без изменений лежат в `SourceArt/Inspection/Arithmetic`; импортированные UI-текстуры — в `/Game/NENS/UI/Arithmetic/Textures`. Отображение слегка обрезает пустое верхнее поле одинаково у всех кадров. Переходы и ввод работают в текущей комнате, без загрузки отдельной карты.

Служебный `Tools/configure_arithmetic_computer.py` импортирует данные, настраивает Blueprint и заменяет только соответствующие декоративные спрайты на `L_MainRooms`. Перед изменениями сохранена копия пользовательской карты `Saved/ArithmeticPuzzle/L_MainRooms-before.umap`. Повторно запускать скрипт для обычной расстановки не требуется.

Проверки: `NENS.Arithmetic` — правила ответа, реальный ввод в PIE, порядок кадров, закрытие во время анимации и с фокусом в поле, повторная попытка, общий прогресс и компактное окно. Остальные `NENS.*` проверяют провода на новой электростанции и обновлённые требования главного компьютера. Отчёты и снимки — в `Saved/ArithmeticPuzzle`.
