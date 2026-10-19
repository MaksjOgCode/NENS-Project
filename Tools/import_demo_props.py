"""Import the demo's decorative props via Unreal API. Existing assets are left alone.

Run UnrealEditor-Cmd -run=pythonscript -script=<this file> -NullRHI
with -ddc=InstalledNoZenLocalFallback and the interactive editor closed.
"""
import json
import uuid
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir())
out = project / 'Saved/PropsImport'
out.mkdir(parents=True, exist_ok=True)
manifest = json.loads((project / 'Tools/props_manifest.json').read_text(encoding='utf-8-sig'))
root = '/Game/NENS/Environment/Interior/Props'
tools = unreal.AssetToolsHelpers.get_asset_tools()
material = unreal.load_asset('/Paper2D/TranslucentUnlitSpriteMaterial')
assert material
report = []
sprites, blueprints = [], []

for entry in manifest:
    name = entry['name']
    texture_path = root + '/Textures/T_Props_' + name
    sprite_path = root + '/Sprites/SPR_Props_' + name
    blueprint_path = root + '/Blueprints/BP_Props_' + name
    texture = unreal.load_asset(texture_path) if unreal.EditorAssetLibrary.does_asset_exist(texture_path) else None
    if texture is None:
        task = unreal.AssetImportTask()
        for key, value in [('filename', str(project / entry['source'])), ('destination_path', root+'/Textures'), ('destination_name', 'T_Props_'+name), ('automated',True), ('save',False), ('replace_existing',False)]:
            task.set_editor_property(key, value)
        tools.import_asset_tasks([task])
        texture = unreal.load_asset(texture_path)
        assert isinstance(texture, unreal.Texture2D), texture_path
        for key, value in [('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT), ('compression_no_alpha',False), ('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD), ('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS), ('filter',unreal.TextureFilter.TF_BILINEAR), ('srgb',True), ('max_texture_size',2048), ('address_x',unreal.TextureAddress.TA_CLAMP), ('address_y',unreal.TextureAddress.TA_CLAMP)]:
            texture.set_editor_property(key, value)
        assert unreal.EditorAssetLibrary.save_loaded_asset(texture)

    sprite = unreal.load_asset(sprite_path) if unreal.EditorAssetLibrary.does_asset_exist(sprite_path) else None
    x0,y0,x1,y1 = entry['bounds']
    width,height = x1-x0,y1-y0
    assert width > 0 and height > 0
    ppu = (width if entry['axis']=='width' else height) / entry['world_size']
    if sprite is None:
        sprite = tools.create_asset('SPR_Props_'+name, root+'/Sprites', unreal.PaperSprite, unreal.PaperSpriteFactory())
        assert isinstance(sprite,unreal.PaperSprite), sprite_path
        sprite.set_editor_property('source_texture',texture)
        sprite.set_editor_property('source_uv',unreal.Vector2D(x0,y0))
        sprite.set_editor_property('source_dimension',unreal.Vector2D(width,height))
        sprite.set_editor_property('pixels_per_unreal_unit',ppu)
        sprite.set_editor_property('pivot_mode',unreal.SpritePivotMode.BOTTOM_CENTER)
        sprite.set_editor_property('sprite_collision_domain',unreal.SpriteCollisionMode.NONE)
        sprite.set_editor_property('default_material',material)
        assert unreal.EditorAssetLibrary.save_loaded_asset(sprite)
    assert sprite.get_editor_property('source_texture') == texture
    sprites.append(sprite)

    bp = unreal.load_asset(blueprint_path) if unreal.EditorAssetLibrary.does_asset_exist(blueprint_path) else None
    if bp is None:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property('parent_class',unreal.Actor)
        bp = tools.create_asset('BP_Props_'+name,root+'/Blueprints',unreal.Blueprint,factory)
    if unreal.EditorAssetLibrary.get_metadata_tag(bp,'NENSPropsLayout') != 'XY1':
        # A child sprite keeps its rotation when the actor is dragged into a level.
        # A rotated native root sprite would be reset by the actor's spawn transform.
        unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.Actor)
        subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        library = unreal.SubobjectDataBlueprintFunctionLibrary
        handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
        parent,reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=handles[0],new_class=unreal.SceneComponent,blueprint_context=bp))
        assert not str(reason), str(reason)
        assert subsystem.rename_subobject(parent,unreal.Text('PropRoot'))
        assert subsystem.make_new_scene_root(handles[0],parent,bp)
        handle,reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=parent,new_class=unreal.PaperSpriteComponent,blueprint_context=bp))
        assert not str(reason), str(reason)
        assert subsystem.rename_subobject(handle,unreal.Text('Sprite_Visual'))
        component = library.get_object_for_blueprint(library.get_data(handle),bp)
        assert component.set_sprite(sprite)
        component.set_editor_property('relative_rotation',unreal.Rotator(pitch=0,yaw=90,roll=-90))
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        unreal.EditorAssetLibrary.set_metadata_tag(bp,'NENSPropsLayout','XY1')
        validation = json.loads(unreal.InspectionEditorLibrary.compile_and_save(bp))
        assert validation['success'] and not validation['warnings'], validation
    blueprints.append(bp)
    report.append(dict(name=name, texture=texture_path, sprite=sprite_path, blueprint=blueprint_path, pixels_per_unit=ppu, world_width=width/ppu,world_height=height/ppu))
    unreal.log('NENS_PROP_READY '+name)

# Shared collections are ordinary project text files. Write them for Git directly:
# the editor's collection-save API requires an enabled revision-control provider.
# Loading/using these collections does not require that provider.
for name,assets in [('NENS_Demo_Props_Sprites',sprites),('NENS_Demo_Props_Ready',blueprints)]:
    path = project / 'Content/Collections' / (name+'.collection')
    if not path.exists():
        path.parent.mkdir(parents=True,exist_ok=True)
        header = 'FileVersion:2\nType:Static\nGuid:'+str(uuid.uuid4())+'\nParentGuid:00000000-0000-0000-0000-000000000000\nIsAutoGenerated:false\n\n'
        path.write_text(header+'\n'.join(sorted(a.get_path_name() for a in assets))+'\n',encoding='utf-8')

(out/'import-result.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
assert len(report)==18
unreal.log('NENS_PROPS_IMPORT_OK: 18 textures, 18 sprites, 18 placement Blueprints, 2 collections')
