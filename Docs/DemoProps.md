# Пропсы для демо

Добавлены 18 текстур, 18 спрайтов и 18 готовых декоративных Blueprint. Всё находится в **Content/NENS/Environment/Interior/Props**: `Textures`, `Sprites`, `Blueprints`. Канонические PNG для повторного импорта лежат в `SourceArt/Props`; исходники на рабочем столе не изменены.

## Список перед глазами

1. Открой **Window → Content Browser → Content Browser 2** и закрепи эту панель сбоку редактора.
2. В левой панели Sources открой **Collections**. Если панель скрыта, включи её кнопкой Show Sources Panel.
3. Выбери **NENS_Demo_Props_Sprites** — получишь только 18 спрайтов без текстур и прочих ассетов. Увеличь миниатюры через Settings/Thumbnail Size.
4. Для быстрой расстановки выбери **NENS_Demo_Props_Ready** и перетаскивай Blueprint на карту. В них уже задан поворот под нашу XY-сцену; нет взаимодействия по E и блокирующей коллизии.
5. Нажми замок Content Browser, чтобы поиск других ассетов через Ctrl+B не переключал эту панель.

Коллекции общие, лежат в `Content/Collections` и передаются через Git. Запасной вариант: открыть папку `Props/Sprites` и добавить её в Favorites. Для просмотра всех спрайтов проекта выбери папку NENS и фильтр **Paper2D → Paper Sprite**.

## Размещение

Готовые `BP_Props_…` сохраняют соотношение сторон и имеют начальные размеры по назначению предмета: мелкие бумаги/бутылка не размером с мебель, растения и мебель сопоставимы с комнатой. Размер конкретного экземпляра меняй через равномерный Scale.

Если перетаскиваешь именно `SPR_…`, а не готовый BP, у созданного PaperSpriteActor выставь **Rotation X=-90, Y=0, Z=90** (Roll=-90, Pitch=0, Yaw=90). Такой поворот используется у существующей мебели. По Z выбирай слой сцены: плоские мелочи можно положить чуть выше пола, крупные пропсы — на слой мебели. Pivot находится внизу по центру видимой рамки спрайта.

Пропсы декоративные: для мебели, которая должна мешать проходу, добавляй отдельный простой Blocking Volume по основанию предмета. Автоматическая коллизия по всему рисунку отключена.

## Настройки и имена

Текстуры сохраняют прозрачность и sRGB, используют Bilinear, Clamp, NoMipmaps и максимальный размер 2048. Материал спрайтов — существующий `TranslucentUnlitSpriteMaterial`. Лишние прозрачные поля убраны рамкой Source UV/Source Dimension спрайта; PNG не обрезались. Pixels per Unit подобран под начальный размер, масштабирование равномерное.

| Исходное имя без T_ | Новое имя после T_Props_/SPR_Props_/BP_Props_ |
|---|---|
| Proops_Metall_Shelf_Front | MetalShelf_Front |
| Proops_Metall_Shelf_Side | MetalShelf_Side |
| Proops_Notes | Notes |
| Proops_Pillar | Pillar |
| Proops_Printing_Desk | TypewriterDesk |
| Proops_Rubbish_0 | CrumpledPaper |
| Proops_Rubbish_1 | Pen |
| Proops_Sofa | Sofa_Front |
| Proops_SIdeSofa | Sofa_Corner |
| Proops_SofaBack | Sofa_Back |
| Proops_Stairs | Stairs |
| Props_Ficus_0 / Props_Ficus_1 | Ficus_0 / Ficus_1 |
| Furniture_Pickme_Box | PickMeBox |
| Proops_Bottle | Bottle |
| Proops_Coil | Coil |
| Proops_DataComputer | DataComputer |
| Proops_Metall_Box | MetalCabinet |

Первоначальный импорт описан в `Tools/import_demo_props.py`, имена/рамки/размеры — в `Tools/props_manifest.json`. Для обычной расстановки запускать скрипт не требуется.

Минимальная проверка: повторная загрузка всех 54 ассетов, создание 18 экземпляров с правильной XY-ориентацией и ссылками на спрайты, наличие по 18 элементов в обеих общих коллекциях. Blueprint скомпилированы без ошибок/предупреждений компилятора. Хеши всех существовавших Content-ассетов остались прежними. Отчёты: `Saved/PropsImport/import-result.json` и `verification.json`. Полный набор игровых тестов и cook не запускались.
