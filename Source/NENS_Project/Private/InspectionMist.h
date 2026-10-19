#pragma once

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/DrawElementTypes.h"
#include "Rendering/RenderingCommon.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

/** Soft animated smoke drawn as a vertex-color mesh; no fog texture or material asset. */
class SInspectionMist : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SInspectionMist) : _Strength(.42f) {}
        SLATE_ARGUMENT(float, Strength)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Strength = FMath::Clamp(Args._Strength, 0.f, 1.f); }
    void SetTime(float InTime) { Time = InTime; Invalidate(EInvalidateWidgetReason::Paint); }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1,1); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
    {
        if (Strength <= 0) return Layer;
        constexpr int32 Columns = 48, Rows = 14;
        TArray<FSlateVertex> Vertices;
        TArray<SlateIndex> Indices;
        Vertices.Reserve((Columns + 1) * (Rows + 1)); Indices.Reserve(Columns * Rows * 6);
        const FVector2f Size(Geometry.GetLocalSize());
        const FLinearColor Tint = Style.GetColorAndOpacityTint();
        for (int32 Row = 0; Row <= Rows; ++Row)
            for (int32 Column = 0; Column <= Columns; ++Column)
            {
                const float X = static_cast<float>(Column) / Columns, Y = static_cast<float>(Row) / Rows;
                const float Depth = FMath::Clamp(Y / (.82f + .13f * FMath::Sin(X * 8 - Time)), 0.f, 1.f);
                const float Wisps = .52f + .20f * FMath::Sin(X * 9 - Time + Y * 3)
                    + .14f * FMath::Sin(X * 20 + Time * .7f - Y * 5)
                    + .08f * FMath::Cos(X * 33 - Time * .4f + Y * 8);
                const float Alpha = Strength * Wisps * FMath::Square(1 - Depth);
                const FColor Color = (FLinearColor(.012f,.014f,.018f, Alpha) * Tint).ToFColor(true);
                Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Geometry.GetAccumulatedRenderTransform(),
                    FVector2f(X * Size.X, Y * Size.Y), FVector2f(.5f,.5f), Color));
                if (Row < Rows && Column < Columns)
                {
                    const SlateIndex A = Row * (Columns + 1) + Column, B = A + Columns + 1;
                    Indices.Append({A, static_cast<SlateIndex>(A + 1), B, B, static_cast<SlateIndex>(A + 1), static_cast<SlateIndex>(B + 1)});
                }
            }
        const auto Resource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));
        FSlateDrawElement::MakeCustomVerts(Elements, Layer, Resource, Vertices, Indices, nullptr, 0, 0);
        return Layer;
    }
private:
    float Strength = .42f, Time = 0;
};
