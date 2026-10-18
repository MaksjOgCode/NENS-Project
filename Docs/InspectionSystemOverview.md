# Как устроена система осмотра и как её менять

Это обзор текущего проекта после добавления осмотра шкафа и исправления мерцания E. Существующие Blueprint можно редактировать вручную. Автоматизация создала и соединила обычные ноды; при каждом Play они не генерируются заново.

Система теперь состоит из Blueprint, C++ и ресурсов данных. Содержимое конкретной сцены меняется в редакторе. Общая логика открытия окна, ввода и сбора написана на C++ и доступна в папке `Source`.

## 1. Что осталось от твоей системы

Осталась прежняя цепочка:

```text
Sphere_Awareness → регистрация объекта, показ E и кольца
Sphere_Interaction → допустимое расстояние для взаимодействия
E → BPC_Interaction.TryInteract → BPI_Interactable.Interact(Interactor)
```

`BP_PlayerCharacter`, привязка E и движение персонажа не переделаны. `BPI_Interactable` сохранил прежний вызов `Interact` с параметром `Interactor`. Материал кольца и его Timeline остались в Blueprint.

Добавленные проверки отвечают на два новых вопроса: «этот объект уже полностью собран?» и «игрок сейчас находится в окне осмотра?». Они не превращают все двери, столы и компьютеры в собираемые объекты.

## 2. Где искать ассеты в Content Browser

В редакторе `/Game` соответствует папке `Content` на диске. Например, `/Game/NENS/UI/Inspection` — это `C:\NENS-Project\Content\NENS\UI\Inspection`.

| Что открыть | Папка в Content Browser | За что отвечает |
|---|---|---|
| `BP_InteractableBase` | `/Game/NENS/Gameplay/Interaction/Blueprints` | Зоны, кольцо, компонент подсказки E, общие функции показа |
| `BPC_Interaction` | `/Game/NENS/Gameplay/Interaction/Components` | Список соседних объектов, выбор ближайшей цели, `TryInteract` |
| `BPI_Interactable` | `/Game/NENS/Gameplay/Interaction/Interfaces` | Общий вызов взаимодействия |
| `BP_Wardrobe` | `/Game/NENS/Environment/Interior/Blueprints` | Подключение осмотра к конкретному шкафу и скрытие индикаторов после завершения |
| `WBP_InteractionPrompt` | `/Game/NENS/UI/Interaction` | Изображение E и анимация появления/скрытия |
| `WBP_InspectionScene` | `/Game/NENS/UI/Inspection` | Компоновка полноэкранного осмотра |
| `DA_Inspection_Wardrobe` | `/Game/NENS/UI/Inspection` | Фон шкафа, перечень предметов, их координаты и размеры |
| `T_Inspection_Wardrobe`, `T_PunchCard` | `/Game/NENS/UI/Inspection/Textures` | Изображения фона и перфокарты |

## 3. Что именно изменено в Blueprint

### BP_Wardrobe

В **Components** добавлен компонент с именем `Searchable`. Его класс — C++ `SearchableComponent`. Выдели компонент: в **Details → Inspection** находятся `Scene` и `Scene Widget Class`.

Сейчас назначены:

```text
Scene              = DA_Inspection_Wardrobe
Scene Widget Class = WBP_InspectionScene
```

В **EventGraph** рабочая цепочка начинается с `Event Interact`:

```text
Event Interact
    → Open Inspection
        Target     = Self
        Interactor = вход Interactor из события
```

`Open Inspection` — нативная C++-функция, представленная обычной нодой. Она находит `Searchable` у шкафа и просит открыть его сцену для взаимодействующего игрока. Старый демонстрационный `Print String` может быть виден отдельно, но его провод выполнения отключён.

Ниже есть событие компонента `On Inspection Closed (Searchable)`:

```text
On Inspection Closed
    → Branch: Completed?
        True → SetAwarenessVisible(false)
             → Widget_InteractionPrompt.SetHiddenInGame(true)
             → Ring_Visual.SetHiddenInGame(true)
```

Этот участок определяет, что происходит с объектом после закрытия. Сюда можно добавлять звук, смену спрайта или другое поведение шкафа. Сам Actor шкафа не скрывается.

### BP_InteractableBase

В `SetAwarenessVisible` запрос показа дополнительно проверяет `Is Interaction Available(Self)`:

```text
Фактически показывать = IsVisible AND IsInteractionAvailable(Self)
```

Если у объекта нет `Searchable`, он доступен по прежним правилам. Если компонент есть и всё собрано, проверка возвращает false.

После проверки осталась твоя цепочка: записать `bAwarenessVisible`, обновить кольцо, передать состояние подсказке E. Анимация кольца, `RingMID`, `TL_RingFade` и материал по-прежнему находятся здесь.

### BPC_Interaction

В `UpdateInteractionTarget` после проверки существования кандидата добавлена проверка его доступности. Завершённый шкаф не участвует в сравнении расстояний.

В `TryInteract` перед отправкой сообщения `Interact` проверяются доступность выбранного объекта и отсутствие активного осмотра. Твой расчёт расстояния и список `NearbyInteractables` остались на месте.

### WBP_InteractionPrompt

В функции `SetPromptFocused` добавлена Boolean-переменная `bPromptFocused`. Теперь анимация запускается только при смене желаемого состояния:

```text
IsFocused == bPromptFocused → ничего не делать
IsFocused != bPromptFocused → записать новое состояние
                           → Play Forward либо Play Reverse
```

Это исправляет мерцание: второй запрос «скрыть» больше не запускает закончившийся Reverse с видимого конца анимации. Саму `Anim_PromptReveal`, её длительность, прозрачность, масштаб и картинку можно менять в Designer/Animations, как раньше.

## 4. Где находится новая логика C++

Исходники игры лежат в `C:\NENS-Project\Source\NENS_Project`:

| Файлы в Public / Private | Простое объяснение |
|---|---|
| `InspectionSceneData.h/.cpp` | Описывают настройки сцены и проверяют их корректность |
| `SearchableComponent.h/.cpp` | Хранят прогресс одного конкретного шкафа |
| `InspectionSubsystem.h/.cpp` | Управляют одним открытым осмотром у локального игрока, курсором и вводом |
| `InspectionSceneWidget.h/.cpp` | Заполняют окно фоном и кнопками предметов, обрабатывают клик и Escape |
| `InspectionBlueprintLibrary.h/.cpp` | Предоставляют ноды `Open Inspection`, `Is Interaction Available`, `Is Interaction Input Allowed` |

`.h` объявляет класс, доступные свойства и функции. `.cpp` содержит выполняемые действия. Префикс `U` в C++ (`USearchableComponent`) относится к именованию Unreal; в редакторе он обычно не отображается.

Отдельные Blueprint-ассеты `BPC_Searchable`, `BPC_Inspection`, `WBP_InspectionPickup` и `ST_InspectionPickup` не создавались. Ранее эти имена были предложениями. В реализации компонент и структура предмета стали C++-классом `USearchableComponent` и структурой `FInspectionPickup`; кнопка предмета — `UInspectionPickupButton` в файлах `InspectionSceneWidget`.

### SearchableComponent: память конкретного шкафа

`CollectedPickupIDs` — множество уже собранных размещений. Оно находится в компоненте шкафа, поэтому удаление окна осмотра не удаляет прогресс.

`TryCollect(PickupID)` выполняет действия в таком порядке:

1. Проверяет данные, существование предмета и отсутствие повторного сбора.
2. Записывает PickupID в прогресс.
3. Вызывает событие `On Item Collected(PickupID, ItemID)`.
4. Возвращает true; только после этого окно скрывает предмет.

`IsComplete()` возвращает true, когда собраны все размещения непустой корректной сцены. Два экземпляра шкафа имеют разные компоненты и разные множества собранных ID, даже если у них один Data Asset.

Прогресс живёт столько же, сколько этот экземпляр шкафа. Повторный вход в окно его сохраняет; Stop/Play, уничтожение шкафа или перезагрузка уровня с созданием нового экземпляра сбросят его. SaveGame пока нет.

### InspectionSubsystem: открытое окно и управление

Subsystem здесь — автоматически создаваемый Unreal объект для локального игрока. Вручную добавлять новый компонент персонажу для него не нужно.

При `Open` он проверяет сцену, создаёт окно, останавливает движение, блокирует ввод движения/поворота, показывает курсор и включает UI-only input. Поэтому нажатия не проходят к игровым командам. Второе окно одновременно не открывается.

При `Close` он удаляет окно, восстанавливает управление и прежнее состояние курсора, затем сообщает компоненту шкафа о закрытии. Компонент передаёт в Blueprint значение `Completed`.

## 5. Почему у окна осмотра почти пустой EventGraph

Открой `WBP_InspectionScene → Class Settings`: родитель — `InspectionSceneWidget`. Поведение наследуется из C++, поэтому большая часть логики не нарисована нодами в этом WBP.

В Designer находится общая разметка:

```text
Canvas_Root
├─ Image_Backdrop          — чёрное заполнение вокруг сцены
├─ ScaleBox_Scene          — масштабирует фон и предметы совместно
│  └─ SizeBox_Scene        — размер макета из данных
│     └─ Canvas_Scene      — слой фона и создаваемых предметов
│        └─ Image_Background
└─ Text_ExitHint           — «Esc — выйти»
```

Кнопки предметов добавляются в `Canvas_Scene` **во время игры** из массива Pickups. Поэтому отсутствие перфокарты в дереве Designer нормально. Её положение задаётся в Data Asset.

Сохраняй имена `Canvas_Scene`, `Image_Background`, `SizeBox_Scene`: C++ находит эти три элемента по имени. Обычное переименование в Designer не исправит строку в исходнике автоматически. Также сохраняй родителя `InspectionSceneWidget`, пока логика не перенесена в другой класс.

Escape обрабатывается в C++ `NativeOnPreviewKeyDown`, а не в Input Action персонажа. После открытия ввод принадлежит UI; добавленная в персонажа обработка Escape не заменит этот механизм.

## 6. Как менять содержимое без программирования

Начинай с `DA_Inspection_Wardrobe`. Это Data Asset — сохранённые настройки, которые читает общий механизм. Он сам не содержит нод и не хранит прогресс.

Текущие значения:

```text
Background  = T_Inspection_Wardrobe
Canvas Size = (812, 812)
Pickups[0]:
    Pickup ID = Wardrobe_PunchCard
    Item ID   = PunchCard
    Image     = T_PunchCard
    Position  = (316, 532)
    Size      = (180, 108)
```

Position — верхний левый угол прямоугольника предмета: X вправо, Y вниз. Это координаты макета 812×812, а не экрана монитора. ScaleBox масштабирует всё вместе. Увеличив Size до `(240, 144)`, ты увеличишь перфокарту с сохранением примерно тех же пропорций; чтобы оставить её по центру, сдвинь Position.X с 316 до 286.

PickupID — «какое именно размещение собрали», ItemID — «что это за тип предмета». Две перфокарты могут иметь одинаковый ItemID `PunchCard`, но разные PickupID: `Card_Left` и `Card_Right`. Иначе невозможно отличить сбор одной от сбора второй.

Для новой сцены дублируй **Data Asset**, назначь другой фон и заполни Pickups. Для нового предмета добавь элемент массива, укажи уникальный PickupID, ItemID, изображение, позицию и размер. Механизм сам создаст нужное количество кнопок. Прямоугольник должен помещаться в Canvas Size; пустая сцена не считается завершённой.

Назначение одного Data Asset нескольким шкафам разделяет настройки, но не прогресс. Изменение этого Data Asset изменит расположение предметов у всех использующих его шкафов. Для разного содержимого создай разные Data Asset.

## 7. Можно ли исправлять баги в Blueprint

Да. Вручную можно редактировать EventGraph шкафа, выбор цели в BPC, зоны, коллизии, кольцо, функцию показа E и анимации. После таких изменений достаточно Compile и Save в Unreal.

Нативная нода выглядит как обычный вызов, но её тело находится в `.cpp`. Текущие функции не предоставляют Blueprint-Override для всей внутренней логики сбора/открытия. Поэтому изменение алгоритма `TryCollect`, блокировки ввода, автоматического создания кнопок или обработчика Escape требует правки C++, либо отдельного добавления точки расширения для Blueprint. Создание одноимённой функции в WBP не заменяет нативную реализацию.

События `On Item Collected` и `On Inspection Closed` уже доступны в Blueprint. Через них можно добавлять реакцию на результат, не меняя C++: звук, Print String, запись игрового флага, смену спрайта. Для будущего инвентаря есть событие ItemID; глобального списка полученных предметов сейчас нет. Если инвентарь сможет отклонять выдачу предмета, порядок подтверждения сбора потребуется расширить в C++.

После изменения C++ самый понятный рабочий путь — сохранить и закрыть Unreal, собрать `NENS_ProjectEditor Win64 Development`, снова открыть проект. Команда сборки есть в `Docs/InspectionSystem.md`. Редактировать файлы в `Intermediate`, сгенерированные `.generated.h` или DLL в `Binaries` не нужно.

## 8. Где искать причину проблемы

| Симптом | Куда смотреть сначала |
|---|---|
| Объект не показывает E или не выбирается | `Sphere_Awareness`, регистрация в `BPC_Interaction`, расстояние до `Sphere_Interaction`, `Is Interaction Available` |
| E есть, но осмотр не открывается | `BP_Wardrobe.Event Interact`; у Searchable назначены Scene и Scene Widget Class; в Output Log нет ошибки проверки данных |
| Нужно изменить фон, размер или положение перфокарты | `DA_Inspection_Wardrobe`, поля Background / Pickups; не EventGraph шкафа |
| Нужен другой общий вид окна | `WBP_InspectionScene.Designer`; сохраняй обязательные имена и родителя |
| Клик не записывает сбор или предмет появляется повторно | `InspectionSceneWidget.CollectPickup`, затем `SearchableComponent.TryCollect`; проверь PickupID и тот же ли это экземпляр шкафа |
| Escape не закрывает окно / управление не вернулось | `InspectionSceneWidget.cpp` и `InspectionSubsystem.cpp`, фокус окна и вызов Close |
| E снова мелькает | `WBP_InteractionPrompt.SetPromptFocused`, проверка `bPromptFocused`; завершение в EventGraph шкафа |
| После частичного сбора объект пропал из взаимодействия | В `On Inspection Closed` скрытие должно выполняться только при Completed=true |
| Ошибки RingMID | Прежняя инициализация материала в `BP_InteractableBase.BeginPlay`, материал/меш `Ring_Visual`, вызов родительского BeginPlay у потомка |

Для отладки Blueprint поставь breakpoint на `Event Interact` у шкафа и вызови взаимодействие в PIE. Если событие не пришло, ищи раньше — в выборе цели и регистрации. Если пришло, иди дальше по Open Inspection и настройкам Searchable. В Debug Filter выбирай фактический экземпляр шкафа на уровне, чтобы смотреть его состояние.

## 9. Зачем добавлены Editor-модуль и Python

`Source/NENSInspectionEditor` и скрипты в `Tools` нужны для чтения графов, первоначальной настройки ассетов, применения исправлений и тестов. Они не обрабатывают игровой клик и не запускаются при каждом Play.

`configure_inspection.py` приводит некоторые настройки и разметку к запрограммированному виду. `fix_inspection_prompt.py` применяет конкретное исправление. После ручных правок не требуется запускать их для «синхронизации»: при повторном запуске служебный скрипт может вернуть часть своих настроек. Обычный путь работы — редактировать нужный ресурс в Unreal и сохранять его.

Проект теперь содержит C++. При передаче коллеге нужны ассеты, папка Source и изменённый `.uproject`; DLL не заменяет исходники. На другой машине C++-модули должны быть собраны совместимым Unreal и компилятором.

## 10. С чего начать знакомство в редакторе

1. `BP_Wardrobe → Components → Searchable`: увидеть две назначенные ссылки.
2. `DA_Inspection_Wardrobe`: сопоставить один элемент Pickups с видимой перфокартой.
3. `BP_Wardrobe → EventGraph`: пройти Event Interact и On Inspection Closed.
4. `WBP_InspectionScene → Designer / Class Settings`: увидеть разметку и C++-родителя.
5. `WBP_InteractionPrompt → SetPromptFocused`: посмотреть защиту от повторной анимации.
6. `BPC_Interaction → UpdateInteractionTarget / TryInteract`: увидеть добавленные проверки доступности.

Так ты увидишь места подключения новой механики к знакомой системе до чтения исходников. Подробности сборки, тестов и резервных копий находятся в `Docs/InspectionSystem.md`.
