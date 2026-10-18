"""Apply the focused, idempotent inspection prompt flicker fix through Unreal."""
import json
from pathlib import Path
import unreal

result = json.loads(unreal.InspectionEditorLibrary.fix_inspection_prompt())
out = Path(unreal.Paths.project_dir()) / 'Saved' / 'InspectionPromptFix'
out.mkdir(parents=True, exist_ok=True)
(out / 'fix-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
unreal.log('NENS_PROMPT_FIX_OK ' + json.dumps(result))
