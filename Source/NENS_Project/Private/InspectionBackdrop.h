#pragma once

#include "Rendering/DrawElements.h"
#include "Rendering/DrawElementTypes.h"
#include "Widgets/SLeafWidget.h"

/** Texture-free charcoal surround. The centered scene is drawn above this widget. */
class SInspectionBackdrop : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SInspectionBackdrop) : _SceneAspect(1.f) {}
        SLATE_ARGUMENT(float, SceneAspect)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args) { SceneAspect = Args._SceneAspect; }

    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1,1); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
    {
        const FVector2f Size(Geometry.GetLocalSize());
        const float Side = FMath::Max(0.f, (Size.X - Size.Y * SceneAspect) * .5f);
        const FLinearColor Tint = Style.GetColorAndOpacityTint();
        const FLinearColor Edge = FLinearColor(.018f,.019f,.021f,1) * Tint;
        const FLinearColor Inner = FLinearColor(.095f,.095f,.09f,1) * Tint;
        TArray<FSlateGradientStop> Stops;
        Stops.Emplace(FVector2f(0,0), Edge);
        Stops.Emplace(FVector2f(FMath::Max(Side, Size.X * .3f), Size.Y * .5f), Inner);
        Stops.Emplace(FVector2f(FMath::Min(Size.X - Side, Size.X * .7f), Size.Y * .5f), Inner);
        Stops.Emplace(Size, Edge);
        FSlateDrawElement::MakeGradient(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Stops), Orient_Horizontal);
        TArray<FSlateGradientStop> Shade;
        Shade.Emplace(FVector2f(0,0), FLinearColor(0,0,0,.24f) * Tint);
        Shade.Emplace(Size * .5f, FLinearColor(0,0,0,0));
        Shade.Emplace(Size, FLinearColor(0,0,0,.24f) * Tint);
        FSlateDrawElement::MakeGradient(Elements, Layer + 1, Geometry.ToPaintGeometry(), MoveTemp(Shade), Orient_Vertical);
        return Layer + 1;
    }
private:
    float SceneAspect = 1;
};
