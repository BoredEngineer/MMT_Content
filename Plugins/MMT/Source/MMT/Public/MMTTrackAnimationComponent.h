//Copyright(c) 2016 Viktor Kuropiatnyk "BoredEngineer"

#pragma once

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SceneComponent.h"
#include "Components/MeshComponent.h"
#include "Containers/UnrealString.h"
#include "Engine/EngineTypes.h"
#include "MMTTrackSplinePointAnimatedStruct.h"
#include "MMTTrackAnimationComponent.generated.h"

UCLASS(ClassGroup = (MMT), meta = (BlueprintSpawnableComponent))
class MMT_API UMMTTrackAnimationComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	
	//Set default values for this component
	UMMTTrackAnimationComponent();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Called from editor on compilation of BP
	virtual void PostLoad() override;
	
	//Called from editor on compilation of BP
	virtual void InitializeComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Array of sprockets, idlers and roadwheels to be animated"))
	TArray<FString> SprocketsIdlersRoadwheelsNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Spline component that defines path of the track"))
	FString TrackSplineComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Instanced mesh component for rendering treads. Set to 'none' if you want to render treads on your own. For example, using multiple ISMs or using completely different method. UpdateTrackAnimation function updates array of local space transforms for each tread which you can get using GetTreadsTransformArray function"))
	FString TreadsInstancedMeshComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Array of spline point indexes and references for their animation"))
	TArray<FTrackSplinePointAnimatedStruct> AnimatedTrackSplinePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Amount of treads on this track"))
	int32 TreadsOnSide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Plays animation in reverse if meshes are rotated 180 degrees"))
	bool bFlipAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Set to true if pivot of your tread mesh is aligned with front connection pin. This approach provides more precise alignment of the treads. When false it's assumed that pivot is in middle of the tread mesh."))
	bool bTreadPivotIsOnPin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Angle between sprocket's teeth. Can be calculated as 360 / total number of teeth"))
	float AngleBetweenSprocketTeeth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "(Optional) Array of default location for track spline control points in case you don't prepared spline in editor"))
	TArray<FVector> TrackSplinePointLocations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "(Optional) Array of default tangents for track spline control points in case you don't prepared spline in editor"))
	TArray<FVector> TrackSplinePointTangents;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings", meta = (ToolTip = "Enable on screen debug output"))
	bool bDebugMode;


	/**
	*	Sets new angular and linear velocity of the sprocket, idlers, roadwheels and treads
	*	@param AngularVelocity 		used to rotate sprockets, idlers and roadwheel meshes.
	*/
	UFUNCTION(BlueprintCallable, Category = "MMT Track Animation Component")
	void SetTrackPartsAngularVelocity(const float& AngularVelocity);

	/**
	*	Updates animation of track elements. Should be called on each tick for maximum fidelity. Call less frequently for LODs.
	*	@param DeltaTime				Delta time
	*	@return TrackAngularPosition	Angular distance traveled by the track around sprocket in degrees. Think about it as a rope wrapped around the cylinder multiple times but we measure its length not in meters but number of times we wrapped it around cylinder.
	*/
	UFUNCTION(BlueprintCallable, Category = "MMT Track Animation Component")
	void UpdateTrackAnimation(const float& DeltaTime, float& TreadAngularPosition, float& PartsAngularPosition, FRotator& TrackPartsRotator);

	/**
	*	Call from BP constructor to set track spline component into default state and add tread instances to Treads Instanced Mesh component.
	*/
	UFUNCTION(BlueprintCallable, Category = "MMT Track Animation Component")
	void BuildTrackMeshAndSpline();


	/**
	*	Retrieve array of local space transforms of treads, to be used for custom rendering of treads or any other purpose
	*/
	UFUNCTION(BlueprintCallable, Category = "MMT Track Animation Component")
	TArray<FTransform> GetTreadsTransformArray();

	/**
	*	Retrieve angular distance traveled by the track around sprocket in degrees. Think about it as a rope wrapped around the cylinder multiple times but we measure its length not in meters but number of times its wrapped around cylinder.
	*/
	UFUNCTION(BlueprintCallable, Category = "MMT Track Animation Component")
	float GetTrackAngularPosition();

private:
	UPROPERTY()
	TArray<UMeshComponent*> SprocketsIdlersRoadwheelsComponents;

	UPROPERTY()
	USplineComponent* TrackSplineComponent;

	UPROPERTY()
	UInstancedStaticMeshComponent* TreadsInstancedMeshComponent;

	UPROPERTY()
	float TrackPartsAngularVelocityDegrees;

	UPROPERTY()
	float TreadFractionalTravel;

	UPROPERTY()
	float PartsRotationDegrees;
	
	UPROPERTY()
	int TreadOffsetCount;

	UPROPERTY()
	int PartsOffsetCount;

	UPROPERTY()
	int NumberOfFullOffsets;

	UPROPERTY()
	TArray<FTransform> TreadsTransforms;

	// Find reference to named components
	void GetComponentsReference();

	//Calculate transform of the instance using tangent of the spline
	FTransform GetAllignedTransformAlongSplineUsingTangent(const float& Distance);
	
	//Calculate transform of the instance using position of current instance and previous instance
	FTransform GetAllignedTransformAlongSplineUsingPosition(const float& Distance, FVector PositionOfPrevInstance, FVector& OutPositionOfCurrentInstance);

	void CalculateIntAndFracRotationOfTrack(float DeltaPitch);
};