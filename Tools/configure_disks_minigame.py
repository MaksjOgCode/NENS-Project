"""Initial disk/cabinet integration. Run once in Unreal, not before every Play."""
import json
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir())
out = project / 'Saved/DisksIntegration'
out.mkdir(parents=True, exist_ok=True)
texture_path = '/Game/NENS/UI/Inspection/Textures/T_Inspection_Container'
tools = unreal.AssetToolsHelpers.get_asset_tools()
if not unreal.EditorAssetLibrary.does_asset_exist(texture_path):
    task = unreal.AssetImportTask()
    for key, value in [('filename', str(project/'SourceArt/Inspection/T_Inspection_Container.png')),
                       ('destination_path', '/Game/NENS/UI/Inspection/Textures'),
                       ('destination_name', 'T_Inspection_Container'), ('automated', True),
                       ('replace_existing', False), ('save', False)]:
        task.set_editor_property(key, value)
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(texture_path)
    assert texture
    for key, value in [('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI),
                       ('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
                       ('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT),
                       ('filter', unreal.TextureFilter.TF_BILINEAR), ('srgb', True),
                       ('address_x', unreal.TextureAddress.TA_CLAMP), ('address_y', unreal.TextureAddress.TA_CLAMP)]:
        texture.set_editor_property(key, value)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture)

actor_path = '/Game/NENS/Environment/Interior/Blueprints/BP_BedsideCabinet'
if not unreal.EditorAssetLibrary.does_asset_exist(actor_path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.load_class(None,
        '/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase.BP_InteractableBase_C'))
    assert tools.create_asset('BP_BedsideCabinet', actor_path.rsplit('/',1)[0], unreal.Blueprint, factory)

result = json.loads(unreal.InspectionEditorLibrary.configure_disks_minigame())
(out/'configure-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
for path in ['/Game/Minigames/Disks/WBP_DiskBoard', actor_path]:
    bp = unreal.load_asset(path)
    (out/(path.rsplit('/',1)[-1]+'-after.json')).write_text(
        unreal.InspectionEditorLibrary.dump_blueprint(bp), encoding='utf-8')

collection = project/'Content/Collections/NENS_Demo_Props_Ready.collection'
if collection.exists():
    copy = collection.read_text(encoding='utf-8-sig')
    entry = actor_path+'.BP_BedsideCabinet'
    if entry not in copy:
        collection.write_text(copy.rstrip()+'\n'+entry+'\n', encoding='utf-8')
unreal.log('NENS_DISKS_CONFIGURE_OK')
