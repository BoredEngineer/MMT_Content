//Copyright(c) 2017 Viktor Kuropiatnyk "BoredEngineer"

#include "MMTSuspensionStack.h"
//#include "MMTPluginPCH.h"
#include "MMTBPFunctionLibrary.h"
#include "DrawDebugHelpers.h"


// Sets default values for this component's properties
UMMTSuspensionStack::UMMTSuspensionStack()
{
	//Bind async trace delegate
	//TraceDelegate.BindUObject(this, &UMMTSuspensionStack::AsyncTraceDone);
	SprungComponentName = FString("Root");
}

void UMMTSuspensionStack::Initialize()
{
	//Initialize variables again, just in case!
	bContactPointActive = false;
	ContactInducedVelocity = FVector::ZeroVector;
	ContactForceAtPoint = FVector::ZeroVector;
	ContactPointLocation = FVector::ZeroVector;
	ContactPointNormal = FVector::UpVector;
	bSprungMeshComponentSetManually = false;
	bSweepShapeMeshComponentSetManually = false;
	ComponentName = FString("ComponentRefereneFailed");
	ComponentsParentName = FString("ParentRefereneFailed");
	bWarningMessageDisplayed = false;
	WheelHubPositionLS = FVector::ZeroVector;
	PreviousSpringLenght = 1.0f;
	SuspensionForceMagnitude = 0.0f;
	SuspensionForceLS = FVector::ZeroVector;
	SuspensionForceWS = FVector::ZeroVector;
	SuspensionForceScale = 1.0f;


	USceneComponent* TryParent = CastChecked<USceneComponent>(GetOuter());
	if (IsValid(TryParent))
	{
		ParentComponentRef = TryParent;

		GetNamesForComponentAndParent();
		GetNamedComponentsReference();
		PreCalculateParameters();
		GetDefaultWheelPosition();

		//Line Trace default query parameters, called from here to have valid reference to parent
		//LineTraceQueryParameters.bTraceAsyncScene = false;
		LineTraceQueryParameters.bTraceComplex = false;
		LineTraceQueryParameters.bReturnFaceIndex = false;
		LineTraceQueryParameters.bReturnPhysicalMaterial = true;
		LineTraceQueryParameters.AddIgnoredActor(ParentComponentRef->GetOwner());

		//Sphere Trace default query parameters, called from here to have valid reference to parent
		//SphereTraceQueryParameters.bTraceAsyncScene = false;
		SphereTraceQueryParameters.bTraceComplex = false;
		SphereTraceQueryParameters.bReturnFaceIndex = false;
		SphereTraceQueryParameters.bReturnPhysicalMaterial = true;
		SphereTraceQueryParameters.AddIgnoredActor(ParentComponentRef->GetOwner());

		//Shape Sweep default query parameters, called from here to have valid reference to parent
		//ShapeSweepQueryParameters.bTraceAsyncScene = false;
		ShapeSweepQueryParameters.bTraceComplex = false;
		ShapeSweepQueryParameters.bReturnFaceIndex = false;
		ShapeSweepQueryParameters.bReturnPhysicalMaterial = true;
		ShapeSweepQueryParameters.AddIgnoredActor(ParentComponentRef->GetOwner());

	}
	else
	{
		//Disable component to avoid potential null point reference
		SuspensionSettings.bDisabled = true;

		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%Inner Suspension Stack object failed to receive correct parent reference")));
		UE_LOG(LogTemp, Warning, TEXT("Inner Suspension Stack object failed to receive correct parent reference"));
	}

	bContactPointActive = false;
}


void UMMTSuspensionStack::GetNamesForComponentAndParent()
{
		//Get names of component and its parent
		ComponentName = ParentComponentRef->GetName();
		ComponentsParentName = ParentComponentRef->GetOwner()->GetName();
}

//Find and store reference to named components
void UMMTSuspensionStack::GetNamedComponentsReference()
{
	//Sprung mesh reference
	if (!bSprungMeshComponentSetManually)
	{
		if (SprungComponentName != FString("none"))
		{
			SprungMeshComponent = UMMTBPFunctionLibrary::GetMeshComponentReferenceByName(ParentComponentRef, SprungComponentName);
			if (!IsValid(SprungMeshComponent))
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s component failed to find component named '%s' or it's not derived from MeshComponent class"), *ComponentsParentName, *ComponentName, *SprungComponentName));
				UE_LOG(LogTemp, Warning, TEXT("%s->%s component failed to find component named '%s' or it's not derived from MeshComponent class"), *ComponentsParentName, *ComponentName, *SprungComponentName);
			}
		}
		else
		{
			SprungMeshComponent = NULL;
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s component's SprungComponentName property shouldn't be 'none', set proper name for effected component"), *ComponentsParentName, *ComponentName));
			UE_LOG(LogTemp, Warning, TEXT("%s->%s component's EffectedComponentName property shouldn't be 'none', set proper name for effected component"), *ComponentsParentName, *ComponentName);
		}
	}

	//ShapeSweep mesh reference
	if (SuspensionSettings.bCanShapeSweep & !bSweepShapeMeshComponentSetManually)
	{
		if (SuspensionSettings.SweepShapeComponentName != FString("none"))
		{
			SweepShapeMeshComponent = UMMTBPFunctionLibrary::GetMeshComponentReferenceByName(ParentComponentRef, SuspensionSettings.SweepShapeComponentName);
			if (!IsValid(SweepShapeMeshComponent))
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s component failed to find component named '%s' or it's not derived from MeshComponent class"), *ComponentsParentName, *ComponentName, *SuspensionSettings.SweepShapeComponentName));
				UE_LOG(LogTemp, Warning, TEXT("%s->%s component failed to find component named '%s' or it's not derived from MeshComponent class"), *ComponentsParentName, *ComponentName, *SuspensionSettings.SweepShapeComponentName);
			}
		}
		else
		{
			SweepShapeMeshComponent = NULL;
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s component's SweepShapeComponentName property shouldn't be 'none', set proper name for effected component"), *ComponentsParentName, *ComponentName));
			UE_LOG(LogTemp, Warning, TEXT("%s->%s component's EffectedComponentName property shouldn't be 'none', set proper name for effected component"), *ComponentsParentName, *ComponentName);
		}
	}
}

//Recalculate parameters to save performance
void UMMTSuspensionStack::PreCalculateParameters() 
{
	//Shift spring offsets if custom position of the stack is used
	SpringOffsetTopAdjusted = SuspensionSettings.bUseCustomPosition ? SuspensionSettings.StackLocalPosition + SuspensionSettings.SpringTopOffset : SuspensionSettings.SpringTopOffset;
	SpringOffsetBottomAdjusted = SuspensionSettings.bUseCustomPosition ? SuspensionSettings.StackLocalPosition + SuspensionSettings.SpringBottomOffset : SuspensionSettings.SpringBottomOffset;

	//Calculate spring direction in local coordinate system
	SpringDirectionLocal = SpringOffsetBottomAdjusted - SpringOffsetTopAdjusted;
	SpringMaxLenght = SpringDirectionLocal.Size();
	
	//Normalize vector and check if normalization was successful
	if (!SpringDirectionLocal.Normalize())
	{
		SpringDirectionLocal = FVector::UpVector;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s distance between Top and Bottom offsets of the spring shouldn't be zero"), *ComponentsParentName, *ComponentName));
		UE_LOG(LogTemp, Warning, TEXT("%s->%s distance between Top and Bottom offsets of the spring shouldn't be zero"), *ComponentsParentName, *ComponentName);
	}
	
	//Calculate line trace points in local space, taking into account road wheel radius and tread thickness
	LineTraceOffsetTopLS = SpringOffsetTopAdjusted + SpringDirectionLocal * (SuspensionSettings.RoadWheelRadius + SuspensionSettings.TrackThickness);
	LineTraceOffsetBottomLS = SpringOffsetBottomAdjusted + SpringDirectionLocal * (SuspensionSettings.RoadWheelRadius + SuspensionSettings.TrackThickness);

	SphereCheckShape = FCollisionShape::MakeSphere(SuspensionSettings.RoadWheelRadius + SuspensionSettings.TrackThickness);
	
}


//Updates position of the road-wheel, calculates and applies spring force to sprung component
void UMMTSuspensionStack::PhysicsUpdate(const float& DeltaTime)
{
	if (!SuspensionSettings.bDisabled)
	{
		//Update world space transform of parent component
		ReferenceFrameTransform = UMMTBPFunctionLibrary::MMTGetTransformComponent(ParentComponentRef, NAME_None);

		UpdateWheelHubPosition();

		CalculateAndApplySuspensionForce(DeltaTime);
	}
}


void UMMTSuspensionStack::LineTraceForContact()
{
	//Clean results
	FHitResult LineTraceOutHit = FHitResult(ForceInit);

	//Transform points into world space using component's transform
	FVector LineTraceStart = ReferenceFrameTransform.TransformPosition(LineTraceOffsetTopLS);
	FVector LineTraceEnd = ReferenceFrameTransform.TransformPosition(LineTraceOffsetBottomLS);

	//Do line trace
	bContactPointActive = ParentComponentRef->GetWorld()->LineTraceSingleByChannel(LineTraceOutHit, LineTraceStart, LineTraceEnd, SuspensionSettings.RayCheckTraceChannel,
		LineTraceQueryParameters, LineTraceResponseParameters);
		//ParentComponentRef->GetWorld()->LineTraceSingleByChannel(LineTraceOutHit, LineTraceStart, LineTraceEnd, SuspensionSettings.RayCheckTraceChannel,
			//				LineTraceQueryParameters, LineTraceResponseParameters);
	
	//Dirty but assumption is that contact information is never used if bContactPointActive is false.
	if (bContactPointActive)
	{
		ContactPointLocation = LineTraceOutHit.ImpactPoint;
		ContactPointNormal = LineTraceOutHit.ImpactNormal;
		ContactPhysicalMaterial = LineTraceOutHit.PhysMaterial.Get();

		if (SuspensionSettings.bGetContactBodyVelocity)
		{
			if (LineTraceOutHit.Component->IsSimulatingPhysics())
			{
				ContactInducedVelocity = LineTraceOutHit.Component->GetPhysicsLinearVelocityAtPoint(ContactPointLocation);
			}
			else
			{
				ContactInducedVelocity = LineTraceOutHit.Component->ComponentVelocity;
			}
		}
	}

	//Draw debug
	if (SuspensionSettings.bEnableDebugMode)
	{
		DrawDebugLineTrace(bContactPointActive, LineTraceStart, LineTraceEnd, LineTraceOutHit.ImpactPoint, ParentComponentRef->GetWorld());
	}
}


void UMMTSuspensionStack::SphereTraceForContact()
{
	//Clean results
	FHitResult SphereTraceOutHit = FHitResult(ForceInit);

	//Transform points into world space using component's transform
	FVector SphereTraceStart = ReferenceFrameTransform.TransformPosition(SpringOffsetTopAdjusted);
	FVector SphereTraceEnd = ReferenceFrameTransform.TransformPosition(SpringOffsetBottomAdjusted);

	FQuat SphereCheckRotator = FQuat();

	//Do sphere trace
	bContactPointActive = ParentComponentRef->GetWorld()->SweepSingleByChannel(SphereTraceOutHit, SphereTraceStart, SphereTraceEnd, SphereCheckRotator,
		SuspensionSettings.SphereCheckTraceChannel, SphereCheckShape, SphereTraceQueryParameters, SphereTraceResponseParameters);
	
	//Dirty but assumption is that contact information is never used if bContactPointActive is false.
	if (bContactPointActive)
	{
		ContactPointLocation = SphereTraceOutHit.ImpactPoint;
		ContactPointNormal = SphereTraceOutHit.ImpactNormal;
		ContactPhysicalMaterial = SphereTraceOutHit.PhysMaterial.Get();
		TracedHubLocation = SphereTraceOutHit.Location;
		
		if (SuspensionSettings.bGetContactBodyVelocity)
		{
			if (SphereTraceOutHit.Component->IsSimulatingPhysics())
			{
				ContactInducedVelocity = SphereTraceOutHit.Component->GetPhysicsLinearVelocityAtPoint(ContactPointLocation);
			}
			else
			{
				ContactInducedVelocity = SphereTraceOutHit.Component->ComponentVelocity;
			}
		}
	}

	//Draw debug
	if (SuspensionSettings.bEnableDebugMode)
	{
		DrawDebugSphereTrace(bContactPointActive, SphereTraceStart, SphereTraceEnd, TracedHubLocation, SuspensionSettings.RoadWheelRadius + SuspensionSettings.TrackThickness,
			SphereTraceOutHit.ImpactPoint, ParentComponentRef->GetWorld());
	}

}

void UMMTSuspensionStack::ShapeSweepForContact()
{
	//Clean results
	TArray<FHitResult> ShapeSweepOutHits;

	//Transform points into world space using component's transform
	FVector ShapeSweepStart = ReferenceFrameTransform.TransformPosition(SpringOffsetTopAdjusted);
	FVector ShapeSweepEnd = ReferenceFrameTransform.TransformPosition(SpringOffsetBottomAdjusted);

	FQuat ShapeSweepRotator = FQuat();

	if (SuspensionSettings.bRotateAlongTraceVector)
	{
		ShapeSweepRotator = FRotationMatrix::MakeFromZ(ReferenceFrameTransform.TransformVector(SpringOffsetBottomAdjusted - SpringOffsetTopAdjusted)).ToQuat();
	}
	else
	{ 
		ShapeSweepRotator = FRotationMatrix::MakeFromXZ(SweepShapeMeshComponent->GetForwardVector(), SweepShapeMeshComponent->GetUpVector()).ToQuat();
	}
	
	//Do shape sweep
	bContactPointActive = ParentComponentRef->GetWorld()->ComponentSweepMulti(ShapeSweepOutHits, SweepShapeMeshComponent, ShapeSweepStart, ShapeSweepEnd, ShapeSweepRotator,
		ShapeSweepQueryParameters);

	//Dirty but assumption is that contact information is never used if bContactPointActive is false.
	if (bContactPointActive)
	{
		FHitResult ShapeSweepClosestOutHit = ShapeSweepOutHits[0];
		
		ContactPointLocation = ShapeSweepClosestOutHit.ImpactPoint;
		ContactPointNormal = ShapeSweepClosestOutHit.ImpactNormal;
		ContactPhysicalMaterial = ShapeSweepClosestOutHit.PhysMaterial.Get();
		TracedHubLocation = ShapeSweepClosestOutHit.Location;

		if (SuspensionSettings.bGetContactBodyVelocity)
		{
			if (ShapeSweepClosestOutHit.Component->IsSimulatingPhysics())
			{
				ContactInducedVelocity = ShapeSweepClosestOutHit.Component->GetPhysicsLinearVelocityAtPoint(ContactPointLocation);
			}
			else
			{
				ContactInducedVelocity = ShapeSweepClosestOutHit.Component->ComponentVelocity;
			}
		}
	}
	
	//Draw debug
	if (SuspensionSettings.bEnableDebugMode)
	{
		DrawDebugShapeSweep(bContactPointActive, ShapeSweepStart, ShapeSweepEnd, TracedHubLocation, ContactPointLocation, ParentComponentRef->GetWorld());
	}

}

//Find new position of road-wheel according to current simulation mode
void UMMTSuspensionStack::UpdateWheelHubPosition()
{
	if (SuspensionSettings.SimulationMode == ESuspensionSimMode::RayCheck)
	{
		if (!TryAsyncTraceMode)
		{
			LineTraceForContact();
		}
		else
		{
			AsyncLineTraceForContact();
		}

		if (bContactPointActive)
		{
			//Shift traced contact point by the radius of the wheel and thickness of track
			WheelHubPositionLS = ReferenceFrameTransform.InverseTransformPosition(ContactPointLocation) - SpringDirectionLocal * (SuspensionSettings.RoadWheelRadius + SuspensionSettings.TrackThickness);
		}
		else
		{
			//If trace failed wheel hub is assumed to be in lowest possible position of suspension
			WheelHubPositionLS = SpringOffsetBottomAdjusted;
		}

		if (SuspensionSettings.bEnableDebugMode)
		{
			DrawDebugSphere(ParentComponentRef->GetWorld(), ReferenceFrameTransform.TransformPosition(WheelHubPositionLS), 10.0, 9, FColor::Blue, false, 0.0f, 0, 1.0f);
		}
	}
	else
	{
		
		if (SuspensionSettings.SimulationMode == ESuspensionSimMode::SphereCheck)
		{
			if (SuspensionSettings.bCanSphereCheck)
			{
				SphereTraceForContact();

				if (bContactPointActive)
				{
					WheelHubPositionLS = ReferenceFrameTransform.InverseTransformPosition(TracedHubLocation);
				}
				else
				{
					//If trace failed wheel hub is assumed to be in lowest possible position of suspension
					WheelHubPositionLS = SpringOffsetBottomAdjusted;
				}

				if (SuspensionSettings.bEnableDebugMode)
				{
					DrawDebugSphere(ParentComponentRef->GetWorld(), ReferenceFrameTransform.TransformPosition(WheelHubPositionLS), 10.0, 9, FColor::Blue, false, 0.0f, 0, 1.0f);
				}
			}
			else
			{
				if (!bWarningMessageDisplayed)
				{
					bWarningMessageDisplayed = true;
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s Sphere Check mode is enabled but bCanSphereCheck property is set to false."), *ComponentsParentName, *ComponentName));
					UE_LOG(LogTemp, Warning, TEXT("%s->%s Sphere Check mode is enabled but bCanSphereCheck property is set to false."), *ComponentsParentName, *ComponentName);
				}
			}

		}
		else
		{
			if (SuspensionSettings.SimulationMode == ESuspensionSimMode::ShapeSweep)
			{
				if (IsValid(SweepShapeMeshComponent))
				{
					if (SuspensionSettings.bCanShapeSweep)
					{
						ShapeSweepForContact();
						if (bContactPointActive)
						{
							WheelHubPositionLS = ReferenceFrameTransform.InverseTransformPosition(TracedHubLocation);
						}
						else
						{
							//If trace failed wheel hub is assumed to be in lowest possible position of suspension
							WheelHubPositionLS = SpringOffsetBottomAdjusted;
						}

						if (SuspensionSettings.bEnableDebugMode)
						{
							DrawDebugSphere(ParentComponentRef->GetWorld(), ReferenceFrameTransform.TransformPosition(WheelHubPositionLS), 10.0, 9, FColor::Blue, false, 0.0f, 0, 1.0f);
						}
					}
					else
					{
						if (!bWarningMessageDisplayed)
						{
							bWarningMessageDisplayed = true;
							GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s Shape Sweep mode is enabled but bCanShapeSweep property is set to false."), *ComponentsParentName, *ComponentName));
							UE_LOG(LogTemp, Warning, TEXT("%s->%s Shape Sweep mode is enabled but bCanShapeSweep property is set to false."), *ComponentsParentName, *ComponentName);
						}
					}
				}
				else
				{
					if (!bWarningMessageDisplayed)
					{
						bWarningMessageDisplayed = true;
						GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s Shape Sweep mode is enabled but Sweep Shape mesh reference is invalid"), *ComponentsParentName, *ComponentName));
						UE_LOG(LogTemp, Warning, TEXT("%s->%s Shape Sweep mode is enabled but Sweep Shape mesh reference is invalid"), *ComponentsParentName, *ComponentName);
					}
				}

			}
		}
	}
}


void UMMTSuspensionStack::CalculateAndApplySuspensionForce(const float& DeltaTime)
{
	//Calculate spring compression
	float NewSpringLenght = FVector(SpringOffsetTopAdjusted - WheelHubPositionLS).Size();
	float SpringDelta = FMath::Clamp(SpringMaxLenght - NewSpringLenght, 0.0f, SpringMaxLenght);

	//DrawDebugString(ParentComponentRef->GetWorld(), ReferenceFrameTransform.GetLocation(), FString::SanitizeFloat(FVector(LineTraceOffsetTopLS-LineTraceOffsetBottomLS).Size()), 0, FColor::Blue, 0.0f, false);
	//DrawDebugString(ParentComponentRef->GetWorld(), ReferenceFrameTransform.GetLocation()+FVector(0.0f,0.0f,10.0f), FString::SanitizeFloat(SpringMaxLenght), 0, FColor::Red, 0.0f, false);
	//DrawDebugString(ParentComponentRef->GetWorld(), ReferenceFrameTransform.GetLocation(), WheelHubPositionLS.ToString(), 0, FColor::Blue, 0.0f, false);
	//DrawDebugString(ParentComponentRef->GetWorld(), ReferenceFrameTransform.GetLocation()+FVector(0.0f,0.0f,10.0f), SpringOffsetTopAdjusted.ToString(), 0, FColor::Red, 0.0f, false);

	CompressionRatio = FMath::Clamp((NewSpringLenght - 1.0f) / SpringMaxLenght, 0.0f, 1.0f);

	float SpringForce;

	//Spring force curve is defined by two segments. First segment is very steep, it goes from 0 to 1cm of suspension travel and force changes from 0 to full amount of SpringStiffness
	//Second segment starts from 1cm and goes to SpringMaxLenght, on this segment force changes from SpringStiffness to SpringStiffness * SpringMaxOutputRatio
	if (SpringDelta > 1.0f)
	{
		SpringForce = FMath::Lerp(SuspensionSettings.SpringStiffness * SuspensionSettings.SpringMaxOutputRatio, SuspensionSettings.SpringStiffness, CompressionRatio);
	}
	else
	{
		SpringForce = SuspensionSettings.SpringStiffness * SpringDelta;
	}
	//Limit damping to the magnitude of the spring force
	float SpringDamping = FMath::Clamp(SuspensionSettings.SpringDampening * ((NewSpringLenght - PreviousSpringLenght) / DeltaTime), -SpringForce, SpringForce);

	//Limit magnitude of suspension force
	//SuspensionForceMagnitude = FMath::Clamp(SpringForce - Dampening, 0.0f, SuspensionSettings.SpringStiffness * SuspensionSettings.SpringMaxOutputRatio);
	SuspensionForceMagnitude = (SpringForce - SpringDamping) * SuspensionForceScale;

	SuspensionForceLS = SuspensionForceMagnitude * (-1) * SpringDirectionLocal;

	SuspensionForceWS = ReferenceFrameTransform.TransformVector(SuspensionForceLS);

	ContactForceAtPoint = SuspensionForceWS;

	//Apply suspension force
	UMMTBPFunctionLibrary::MMTAddForceAtLocationComponent(SprungMeshComponent, SuspensionForceWS, ReferenceFrameTransform.GetLocation());

		
	if (SuspensionSettings.bEnableDebugMode)
	{
		DrawDebugLine(ParentComponentRef->GetWorld(), ReferenceFrameTransform.GetLocation(), ReferenceFrameTransform.GetLocation() + SuspensionForceWS * 0.0001f, FColor::Blue, false, 0.0f, 50, 1.0f);
		DrawDebugString(ParentComponentRef->GetWorld(), ReferenceFrameTransform.GetLocation(), FString::SanitizeFloat(SuspensionForceMagnitude), 0, FColor::Blue, 0.0f, false);
	}

	//Store last spring length
	PreviousSpringLenght = NewSpringLenght;
}

void UMMTSuspensionStack::ApplyAntiRollForce(float AntiRollForceMagnitude)
{
	FVector AntiRollForceLS = AntiRollForceMagnitude * (-1) * SpringDirectionLocal;
	SuspensionForceLS += AntiRollForceLS;

	FVector AntiRollForceWS = ReferenceFrameTransform.TransformVector(AntiRollForceLS);
	SuspensionForceWS += AntiRollForceWS;

	ContactForceAtPoint = SuspensionForceWS;

	//Apply anti-roll force
	UMMTBPFunctionLibrary::MMTAddForceAtLocationComponent(SprungMeshComponent, AntiRollForceWS, ReferenceFrameTransform.GetLocation());
}

void UMMTSuspensionStack::DrawDebugLineTrace(bool bBlockingHit, FVector Start, FVector End, FVector HitPoint, UWorld *WorldRef)
{
	if (bBlockingHit)
	{
		DrawDebugLine(WorldRef, Start, HitPoint, FColor::Red, false, 0.0f, 1, 1.0f);
		DrawDebugLine(WorldRef, HitPoint, End, FColor::Green, false, 0.0f, 1, 1.0f);
		DrawDebugPoint(WorldRef, HitPoint, 10.0, FColor::Blue, false, 0.0f, 0);
	}
	else
	{
		DrawDebugLine(WorldRef, Start, End, FColor::Yellow, false, 0.0f, 0, 1.0f);
	}
}

void UMMTSuspensionStack::DrawDebugSphereTrace(bool bBlockingHit, FVector TraceStart, FVector TraceEnd, FVector HitTraceLocation, float SphereRadius, FVector HitPoint, UWorld *WorldRef)
{
	FVector CapsuleDirection = TraceEnd - TraceStart;
	FQuat CapsuleRot = FRotationMatrix::MakeFromZ(CapsuleDirection).ToQuat();

	if (bBlockingHit)
	{
		CapsuleDirection = HitTraceLocation - TraceStart;
		float CapsuleLenght = CapsuleDirection.Size();
		FVector CapsuleCenter = TraceStart + CapsuleDirection * 0.5;
		float CapsuleHalfHeight = CapsuleLenght * 0.5f + SphereRadius;
		DrawDebugCapsule(WorldRef, CapsuleCenter, CapsuleHalfHeight, SphereRadius, CapsuleRot, FColor::Red, false, 0.0f, 1, 1.0f);

		CapsuleDirection = TraceEnd - HitTraceLocation;
		CapsuleLenght = CapsuleDirection.Size();
		CapsuleCenter = HitTraceLocation + CapsuleDirection * 0.5;
		CapsuleHalfHeight = CapsuleLenght * 0.5f + SphereRadius;
		DrawDebugCapsule(WorldRef, CapsuleCenter, CapsuleHalfHeight, SphereRadius, CapsuleRot, FColor::Green, false, 0.0f, 1, 1.0f);

		DrawDebugPoint(WorldRef, HitPoint, 10.0, FColor::Blue, false, 0.0f, 0);
	}
	else
	{
		float CapsuleLenght = CapsuleDirection.Size();
		FVector CapsuleCenter = TraceStart + CapsuleDirection * 0.5;
		float CapsuleHalfHeight = CapsuleLenght * 0.5f + SphereRadius;
		DrawDebugCapsule(WorldRef, CapsuleCenter, CapsuleHalfHeight, SphereRadius, CapsuleRot, FColor::Yellow, false, 0.0f, 1, 1.0f);
	}
}

void UMMTSuspensionStack::DrawDebugShapeSweep(bool bBlockingHit, FVector Start, FVector End, FVector Location,FVector HitPoint, UWorld *WorldRef)
{
	if (bBlockingHit)
	{
		DrawDebugLine(WorldRef, Start, Location, FColor::Red, false, 0.0f, 1, 1.0f);
		DrawDebugLine(WorldRef, Location, End, FColor::Green, false, 0.0f, 1, 1.0f);
		DrawDebugPoint(WorldRef, HitPoint, 10.0, FColor::Purple, false, 0.0f, 0);
	}
	else
	{
		DrawDebugLine(WorldRef, Start, End, FColor::Yellow, false, 0.0f, 0, 1.0f);
	}
}

void UMMTSuspensionStack::ToggleDebugMode()
{
	SuspensionSettings.bEnableDebugMode = !SuspensionSettings.bEnableDebugMode;
}

void UMMTSuspensionStack::GetSuspensionForce(float &Magnitude, FVector& WorldSpace, FVector& LocalSpace)
{
	Magnitude = SuspensionForceMagnitude;
	WorldSpace = SuspensionForceWS;
	LocalSpace = SuspensionForceLS;
}

	void UMMTSuspensionStack::GetContactData(bool &bPointActive, FVector& ForceAtPoint, FVector& PointLocation, FVector& SurfaceNormal, class UPhysicalMaterial*& SurfacePhysicalMaterial, FVector& SurfaceVelocity)
{
	bPointActive = bContactPointActive;
	ForceAtPoint = ContactForceAtPoint;
	PointLocation = ContactPointLocation;
	SurfaceNormal = ContactPointNormal;
	SurfacePhysicalMaterial = ContactPhysicalMaterial;
	SurfaceVelocity = ContactInducedVelocity;
}

FVector UMMTSuspensionStack::GetWheelHubPosition(bool bInWorldSpace)
{
	if (bInWorldSpace)
	{
		return ReferenceFrameTransform.TransformPosition(WheelHubPositionLS);
	}
	else
	{
		return WheelHubPositionLS;
	}
}


bool UMMTSuspensionStack::SetSprungComponentReference(UMeshComponent* SprungMeshComponentRef)
{
	if (IsValid(SprungMeshComponentRef))
	{
		SprungMeshComponent = SprungMeshComponentRef;
		bSprungMeshComponentSetManually = true;
		return true;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s manual setting of sprung mesh component failed"), *ComponentsParentName, *ComponentName));
		UE_LOG(LogTemp, Warning, TEXT("%s->%s manual setting of sprung mesh component failed"), *ComponentsParentName, *ComponentName);
		return false;
	}
}


bool UMMTSuspensionStack::SetSweepShapeComponentReference(UMeshComponent* SweepShapeMeshComponentRef)
{
	if (IsValid(SweepShapeMeshComponentRef))
	{
		SweepShapeMeshComponent = SweepShapeMeshComponentRef;
		bSweepShapeMeshComponentSetManually = true;
		return true;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s manual setting of sweep shape mesh component failed"), *ComponentsParentName, *ComponentName));
		UE_LOG(LogTemp, Warning, TEXT("%s->%s manual setting of sweep shape mesh component failed"), *ComponentsParentName, *ComponentName);
		return false;
	}
}

void UMMTSuspensionStack::SetSpringStiffness(float NewSpringStiffness)
{
	SuspensionSettings.SpringStiffness = NewSpringStiffness;
}

void UMMTSuspensionStack::SetSpringOffsets(FVector SpringTopOffset, FVector SpringBottomOffset, bool bUpdateAllParameters)
{
	SuspensionSettings.SpringTopOffset = SpringTopOffset;
	SuspensionSettings.SpringBottomOffset = SpringBottomOffset;

	if (bUpdateAllParameters)
	{
		PreCalculateParameters();
	}
}

void UMMTSuspensionStack::SetSuspensionForceScale(float NewSuspensionForceScale)
{
	SuspensionForceScale = NewSuspensionForceScale;
}

float UMMTSuspensionStack::GetSuspensionForceScale()
{
	return SuspensionForceScale;
}


float UMMTSuspensionStack::GetSuspensionCompressionRatio()
{
	return CompressionRatio;
}

void UMMTSuspensionStack::GetDefaultWheelPosition()
{
	if (IsValid(SweepShapeMeshComponent))
	{
		WheelHubPositionLS = SweepShapeMeshComponent->GetRelativeTransform().GetLocation();
		//WheelHubPositionLS = ReferenceFrameTransform.InverseTransformPosition(SweepShapeMeshComponent->GetComponentTransform().GetLocation());
	}
}


//Functions for async trace, not used but keep them for feature experiments
void UMMTSuspensionStack::AsyncLineTraceForContact()
{
	//Transform points into world space using component's transform
	FVector LineTraceStart = ReferenceFrameTransform.TransformPosition(LineTraceOffsetTopLS);
	FVector LineTraceEnd = ReferenceFrameTransform.TransformPosition(LineTraceOffsetBottomLS);

	//Request async trace
	ParentComponentRef->GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single, LineTraceStart, LineTraceEnd, SuspensionSettings.RayCheckTraceChannel,
		LineTraceQueryParameters, LineTraceResponseParameters, &TraceDelegate, 0);
}

//Process async line trace results
void UMMTSuspensionStack::AsyncTraceDone(const FTraceHandle& TraceHandle, FTraceDatum & TraceData)
{
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Got Async trace callback")));
	//UE_LOG(LogTemp, Warning, TEXT("Got Async trace callback"));

	if (TraceData.OutHits.Num() > 0)
	{
		if (SuspensionSettings.bEnableDebugMode)
		{
			DrawDebugLineTrace(TraceData.OutHits[0].bBlockingHit, TraceData.Start, TraceData.End, TraceData.OutHits[0].ImpactPoint, ParentComponentRef->GetWorld());
		}
	}
	else {}
}


//Process async line trace results
void UMMTSuspensionStack::Test()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s->%s Reporting"), *ComponentsParentName, *ComponentName));
}