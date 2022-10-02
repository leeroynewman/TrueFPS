// Fill out your copyright notice in the Description page of Project Settings.


#include "TrueFPSAnimInstance.h"
#include "TrueFPSCharacter.h"
#include "Kismet/KismetMathLibrary.h"

UTrueFPSAnimInstance::UTrueFPSAnimInstance()
{

}

void UTrueFPSAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

void UTrueFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character) return;
	SetVars(DeltaSeconds);

	if (CurrentWeapon)
	{
		CalculateWeaponSway(DeltaSeconds);
	}

	LastRotation = CameraTransform.Rotator();

	IsDead = Character->GetHealth() <= 0;
}

void UTrueFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	Character = Cast<ATrueFPSCharacter>(TryGetPawnOwner());

	if (Character)
	{
		Character->CurrentWeaponChangeDelegate.AddDynamic(this, &UTrueFPSAnimInstance::CurrentWeaponChanged);
		//CurrentWeaponChanged(Character->CurrentWeapon, nullptr);
	}
}

void UTrueFPSAnimInstance::CurrentWeaponChanged(AWeapon* NewWeapon, const AWeapon* OldWeapon)
{
	CurrentWeapon = NewWeapon;

	if (CurrentWeapon)
	{
		IKProperties = CurrentWeapon->IKProperties;
		HoldType = CurrentWeapon->HoldType;
		SetIKTransforms();
	}
}

void UTrueFPSAnimInstance::SetVars(const float Deltatime)
{
	CameraTransform = FTransform(Character->GetBaseAimRotation(),Character->Camera->GetComponentLocation());

	const FTransform& RootOffset = GetSkelMeshComponent()->GetSocketTransform(TEXT("root"), RTS_Component).Inverse() * GetSkelMeshComponent()->GetSocketTransform(FName("ik_hand_root"));
	RelativeCameraTransform = CameraTransform.GetRelativeTransform(RootOffset);

	const FRotator CharRotation = Character->GetActorRotation();
	const FVector CharForwardVector = UKismetMathLibrary::GetForwardVector(CharRotation);
	const FVector CharSideVector = UKismetMathLibrary::GetRightVector(CharRotation);
	const FVector CharVel = Character->GetVelocity();
	
	Speed.X = FVector::DotProduct(CharVel, CharSideVector);
	Speed.Y = FVector::DotProduct(CharVel, CharForwardVector);

	AimAt = Character->GetAimVector();

	ADSWeight = Character->ADSWeight;
}

void UTrueFPSAnimInstance::CalculateWeaponSway(const float Deltatime)
{
	FVector LocationOffset = FVector::ZeroVector;
	FRotator RotationOffset = FRotator::ZeroRotator;

	constexpr float AngleClamp = 6.0f;
	const FRotator& AddRotation = CameraTransform.Rotator() - LastRotation;
	FRotator AddRotationClamped = FRotator(FMath::ClampAngle(AddRotation.Pitch, -AngleClamp, AngleClamp * 1.5f),
		FMath::ClampAngle(AddRotation.Yaw, -AngleClamp, AngleClamp), 0.f);
	AddRotationClamped.Roll = AddRotationClamped.Yaw * 0.7f;

	AccumulativeRotation += AddRotationClamped;
	AccumulativeRotation = UKismetMathLibrary::RInterpTo(AccumulativeRotation, FRotator::ZeroRotator, Deltatime, 30.f);
	AccumulativeRotationInter = UKismetMathLibrary::RInterpTo(AccumulativeRotationInter, AccumulativeRotation, Deltatime, 5.f);
	
	const FRotator& AccumulationInterpInverse = AccumulativeRotationInter.GetInverse();
	
	RotationOffset += AccumulationInterpInverse;
	LocationOffset += FVector(0.f, AccumulationInterpInverse.Yaw, AccumulationInterpInverse.Pitch) / 6.f;

	float WeightScale = IKProperties.WeightScale;
	
	LocationOffset *= WeightScale;
	RotationOffset.Pitch *= WeightScale;
	RotationOffset.Yaw *= WeightScale;
	RotationOffset.Roll *= WeightScale;
	
	OffsetTransform = FTransform(RotationOffset, LocationOffset);
}

void UTrueFPSAnimInstance::SetIKTransforms()
{
	RHandToSightsTransform = CurrentWeapon->GetSightsWorldTransform().GetRelativeTransform(GetSkelMeshComponent()->GetSocketTransform(FName("hand_r")));
}
