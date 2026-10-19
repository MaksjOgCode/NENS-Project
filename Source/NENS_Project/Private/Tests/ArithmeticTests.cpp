#include "Misc/AutomationTest.h"
#include "ArithmeticPuzzleData.h"
#include "Engine/Texture2D.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArithmeticRulesTest,"NENS.Arithmetic.AnswerAndDefinition",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArithmeticRulesTest::RunTest(const FString&)
{
    auto* Data=NewObject<UArithmeticPuzzleData>();
    TestFalse(TEXT("Missing artwork fails validation"),Data->Validate());
    for (int32 I=0;I<4;++I) Data->Frames.Add(NewObject<UTexture2D>());
    TestTrue(TEXT("Four aligned frames with default timings valid"),Data->Validate());
    for (const TCHAR* Answer : {TEXT("29"),TEXT(" 29 "),TEXT("0029")}) TestTrue(TEXT("Correct integer accepted"),Data->IsCorrect(Answer));
    for (const TCHAR* Answer : {TEXT(""),TEXT("28"),TEXT("30"),TEXT("29.0"),TEXT("2e9"),TEXT("29x"),TEXT("-29"),TEXT("+29"),TEXT("9999999999999999999"),TEXT("2 9")})
        TestFalse(TEXT("Wrong or malformed answer rejected"),Data->IsCorrect(Answer));
    Data->Frames[3]=nullptr; TestFalse(TEXT("Null frame invalid"),Data->Validate());
    Data->Frames[3]=Data->Frames[0]; Data->BlendSeconds=0;
    TestFalse(TEXT("Zero duration invalid"),Data->Validate());
    return true;
}
#endif
