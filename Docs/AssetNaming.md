# Именование ресурсов NENS

Ресурсы проекта размещаем в `/Game/NENS` и группируем по назначению. `/Game` соответствует каталогу `Content` в Unreal. Полная карта текущего переименования находится в [AssetRenameMap.csv](AssetRenameMap.csv).

## Структура каталогов

```text
/Game/NENS/
  Characters/Player/
    Blueprints/
    Input/
    Textures/
    Sprites/
    Animations/
      AS_Player
      ABP_Player
      Flipbooks/
      Sequences/
  Environment/Interior/
    Textures/
    Materials/
    TileSets/
    TileMaps/
  UI/Glasses/
  Maps/Tests/
    Player/
      L_PlayerTest
      GM_PlayerTest
    Lighting/
      L_LightingTest
```

`AS_Player` и `ABP_Player` храним непосредственно в `Animations`. Тестовый `GM_PlayerTest` находится рядом с картой теста игрока.

Исключение: стартовая карта сохраняет путь `/Game/Maps/Startup`. Его используют настройки проекта и инструменты упаковки в `Tools`; перенос требует отдельного обновления этих ссылок.

## Имена и префиксы

Формат: `<Prefix>_<Name>[_<Descriptor>][_<Variant>]`. Используем ASCII, английские слова в PascalCase и одиночные подчёркивания между смысловыми частями: `T_Floor_Wood_Clean`, `SPR_Player_WalkDown_01`. Описатели и варианты добавляем по смыслу. Кадры нумеруем двумя цифрами: `01`, `02`, `03`.

В новых именах не допускаются пробелы, кириллица, нецензурные слова, двойные или завершающие подчёркивания. Избегаем бессодержательных названий вроде `NewAsset`, `Final2` и `Copy`.

| Тип ресурса | Префикс | Пример |
| --- | --- | --- |
| Texture | `T_` | `T_Player_IdleDown_01` |
| Material | `M_` | `M_Wallpaper_Ornate` |
| Material Instance | `MI_` | `MI_Wallpaper_Grimy` |
| PaperTileSet | `TS_` | `TS_Wall_Brick` |
| PaperTileMap | `TM_` | `TM_Interior_Floor` |
| PaperSprite | `SPR_` | `SPR_Player_WalkDown_01` |
| PaperFlipbook | `FB_` | `FB_Player_WalkDown` |
| PaperZD Animation Source | `AS_` | `AS_Player` |
| PaperZD Animation Sequence | `ASQ_` | `ASQ_Player_WalkDown` |
| Blueprint | `BP_` | `BP_PlayerCharacter` |
| Animation Blueprint | `ABP_` | `ABP_Player` |
| Enum | `E_` | `E_PlayerDirection` |
| Input Action | `IA_` | `IA_Move` |
| Input Mapping Context | `IMC_` | `IMC_Player` |
| GameMode Blueprint | `GM_` | `GM_PlayerTest` |
| Level | `L_` | `L_PlayerTest` |
| Widget Blueprint | `WBP_` | `WBP_Glasses` |

Общая схема и базовые префиксы опираются на [рекомендации Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/recommended-asset-naming-conventions-in-unreal-engine-projects). Префиксы Paper2D и PaperZD, включая `AS_`/`ASQ_`, закреплены проектом. Это не обязательный универсальный стандарт Epic: в таблице Epic `AS_` обозначает обычную Animation Sequence, а здесь — PaperZD Animation Source.

## Как переносить и переименовывать

1. Проверьте карту изменений и существование целевых путей. Для пакетов карт учитывайте связанные `_BuiltData`.
2. Выполняйте Rename/Move через Content Browser или Unreal API. Не перемещайте и не переименовывайте `.uasset`/`.umap` средствами ОС.
3. Обновите ссылки, проходящие через redirector, и сохраните все ссылающиеся пакеты. Используйте Fix Up Redirectors в редакторе либо соответствующий Unreal API/commandlet. Удаляйте redirector только после успешного сохранения его referencers. Поведение описано в [документации Epic о redirectors](https://dev.epicgames.com/documentation/unreal-engine/asset-redirectors-in-unreal-engine).
4. Отдельно проверьте пути в `Config`, исходном коде, скриптах `Tools`, строковых литералах, soft paths, настройках стартовых карт и списках упаковки. Исправление redirectors не заменяет аудит таких ссылок.
5. Проверьте загрузку затронутых карт, Blueprints, анимаций, UI и результат упаковки. Результаты проверки фиксируйте отдельно от соглашения об именовании.

Сохраняйте исходные пути импорта текстур. Переименование Unreal-ресурса не должно автоматически менять PNG, их содержимое или сведения об источнике для Reimport.

## Карта переименований

CSV сформирован из `Saved/AssetNamingMigration/manifest.json` и содержит имена пакетов без расширений и суффикса объекта:

| Поле | Значение |
| --- | --- |
| `OldPackage` | Исходный полный путь пакета; сохраняется буквально, включая прежнюю кириллицу. |
| `NewPackage` | Целевой полный путь пакета. |
| `ImplicitBuiltData` | `true` — связанный пакет `_BuiltData`, учитываемый вместе с переносом карты; `false` — явное переименование ресурса. |

Наличие строки в CSV описывает запланированное соответствие путей и само по себе не подтверждает выполнение миграции.
