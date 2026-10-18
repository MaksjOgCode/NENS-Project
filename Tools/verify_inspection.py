"""Compile relevant Blueprint classes without saving unrelated assets."""
import json
from pathlib import Path
import unreal

paths = [
    '/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase',
    '/Game/NENS/Gameplay/Interaction/Components/BPC_Interaction',
    '/Game/NENS/Characters/Player/Blueprints/BP_PlayerCharacter',
    '/Game/NENS/UI/Interaction/WBP_InteractionPrompt',
    '/Game/NENS/UI/Inspection/WBP_InspectionScene',
]
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
paths += [str(a.package_name) for a in registry.get_assets_by_path('/Game/NENS/Environment/Interior/Blueprints')
          if str(a.asset_class_path.asset_name) == 'Blueprint']
results = []
for path in paths:
    bp = unreal.load_asset(path)
    assert bp, path
    result = json.loads(unreal.InspectionEditorLibrary.compile_and_save(bp, False))
    results.append(result)
out = Path(unreal.Paths.project_dir()) / 'Saved' / 'InspectionImplementation'
(out / 'blueprint-validation.json').write_text(json.dumps(results, indent=2, ensure_ascii=False), encoding='utf-8')
assert all(result['success'] for result in results), results
unreal.log('NENS_INSPECTION_BLUEPRINTS_OK ' + str(len(results)))
