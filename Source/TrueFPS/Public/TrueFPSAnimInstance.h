// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Animation/AnimInstance.h"
#include "TrueFPSAnimInstance.generated.h"

/**
 * 
 */

UCLASS()
class TRUEFPS_API UTrueFPSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UTrueFPSAnimInstance();
	
public:
	
	//IK Vars
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	FTransform CameraTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	FTransform RelativeCameraTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	FTransform OffsetTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	FVector AimAt;
	
	//Acumulative Offsets

	UPROPERTY(BlueprintReadWrite, Category = "Anim")
	FRotator AccumulativeRotation;

	UPROPERTY(BlueprintReadWrite, Category="Anim")
	FRotator AccumulativeRotationInter;
	
	//State

	UPROPERTY(BlueprintReadOnly, Category="Anim")
	FRotator LastRotation;

	UPROPERTY(BlueprintReadOnly, Category="Anim")
	FVector2D Speed;
	
	//References
	UPROPERTY(BlueprintReadWrite, Category="Anim")
	class ATrueFPSCharacter* Character;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	FIKProperties IKProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	EWeaponHoldType HoldType;
	
	UPROPERTY(BlueprintReadWrite, Category="Anim")
	class AWeapon* CurrentWeapon;

	UPROPERTY(BlueprintReadWrite, Category="Anim")
	bool IsDead;

	UPROPERTY(BlueprintReadWrite, Category="Anim")
	float ADSWeight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	FTransform RHandToSightsTransform;
	
protected:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeInitializeAnimation() override;

	UFUNCTION()
	virtual void CurrentWeaponChanged(class AWeapon* NewWeapon, const class AWeapon* OldWeapon);
	virtual void SetVars(const float Deltatime);
	virtual void CalculateWeaponSway(const float Deltatime);

	virtual void SetIKTransforms();
};
