// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	IDLE = 0 UMETA(DisplayName = "Idle"),
	ATTACKING = 1 UMETA(DisplayName = "Attacking"),
	RELOADING = 2 UMETA(DisplayName = "Reloading"),
	DEPLOYING = 3 UMETA(DisplayName = "Deploying"),
};

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	PISTOL = 0 UMETA(DisplayName = "Pistol"),
	RIFLE = 1 UMETA(DisplayName = "Rifle"),
	SHOTGUN = 2 UMETA(DisplayName = "Shotgun"),
};

UENUM(BlueprintType)
enum class EWeaponHoldType : uint8
{
	NONE = 0 UMETA(DisplayName = "None"),
	RIFLE = 1 UMETA(DisplayName = "Rifle"),
	SHOTGUN = 2 UMETA(DisplayName = "Shotgun"),
	PISTOL = 3 UMETA(DisplayName = "Pistol"),
};

USTRUCT(BlueprintType)
struct FIKProperties 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UAnimSequence* AnimPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AimOffset = 15.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform CustomOffsetTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeightScale = 1.0f;
};

USTRUCT(BlueprintType)
struct FWeaponConfigurations 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Automatic = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Delay = 0.1f;

	UPROPERTY(EditAnywhere)
	int32 Capacity = -1;

	UPROPERTY(EditAnywhere)
	int32 NumBullets;

	UPROPERTY(EditAnywhere)
	float Spread;

	UPROPERTY(EditAnywhere)
	int32 Distance;

	UPROPERTY(EditAnywhere)
	int32 Damage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAmmoType AmmoType;
};

UCLASS(Abstract)
class TRUEFPS_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	
	UPROPERTY()
	float LastFiredTime;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	void StartPrimaryAttack();

	UFUNCTION(BlueprintCallable)
	void EndPrimaryAttack();

	UFUNCTION(BlueprintCallable)
	void StartHolster();
	
	UFUNCTION(BlueprintCallable)
	void EndHolster();
	
	UFUNCTION(BlueprintCallable)
	void CancelHolster();
	
	UFUNCTION(BlueprintCallable)
	void StartReload();

	UFUNCTION(BlueprintCallable)
	void EndReload();
	
	UFUNCTION(BlueprintCallable)
	void HandlePrimaryAttack();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FTimerHandle TimeHandle_PrimaryAttack;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FTimerHandle TimeHandle_Reload;
	
	//Configurations
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Configurations")
	FWeaponConfigurations Configs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configurations")
	FIKProperties IKProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configurations")
	FTransform PlacementTransform;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component")
	class USkeletalMeshComponent* Mesh;

	UFUNCTION(BlueprintCallable)
	bool CanPrimaryAttack();
	
	//State

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="State")
	EWeaponState CurrentState;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="State")
	int32 AmmoCount;
	
	//Animations

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim")
	EWeaponHoldType HoldType;
	
	//Mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|Mesh")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|Mesh")
	UAnimMontage* ReloadMontage;
	
	//Body
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|Body")
	UAnimMontage* BodyAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|Body")
	UAnimMontage* BodyReloadMontage;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IK")
	FTransform GetSightsWorldTransform() const;
	virtual FORCEINLINE FTransform GetSightsWorldTransform_Implementation() const {return Mesh->GetSocketTransform(FName("socket_sights"));}
};

