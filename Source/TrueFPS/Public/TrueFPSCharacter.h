// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon.h"
#include "Components/TimelineComponent.h"
#include "TrueFPSCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCurrentWeaponChangeDelegate, class AWeapon*, CurrentWeapon, const class AWeapon*, OldWeapon);

UCLASS()
class TRUEFPS_API ATrueFPSCharacter : public ACharacter
{
	GENERATED_BODY()


public:
	// Sets default values for this character's properties
	ATrueFPSCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	void MoveForward(const float Scale);
	void MoveSide(const float Scale);

	void NextWeapon();
	void PreviousWeapon();

	void StartPrimaryAttacking();
	void EndPrimaryAttacking();

	UPROPERTY(EditDefaultsOnly, Category = "Configurations")
	TArray<TSubclassOf<AWeapon>> DefaultWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Configuration|Anim")
	class UCurveFloat* AimingCurve;

	FTimeline AimingTimeline;
	
	UFUNCTION()
	virtual void OnRep_CurrentWeapon(const class AWeapon* OldWeapon);

	UFUNCTION()
	void OnRep_Health();
	
	UFUNCTION(Server, Reliable)
	void Server_SetCurrentWeapon(class AWeapon* NewWeapon);
	virtual void Server_SetCurrentWeapon_Implementation(class AWeapon* NewWeapon);

	//Network Aiming
	
	UFUNCTION(Server, Reliable)
	void Server_Aim(const bool bForward = true);
	virtual FORCEINLINE void Server_Aim_Implementation(const bool bForward)
	{
		Multi_Aim(bForward);
	}

	UFUNCTION(NetMulticast, Reliable)
	void Multi_Aim(const bool bForward);
	virtual void Multi_Aim_Implementation(const bool bForward);

	
	//Network PickupWeapon
	
	UFUNCTION(Server, Reliable)
	void Server_PickupWeapon(AWeapon* TargetWeapon);
	virtual FORCEINLINE void Server_PickupWeapon_Implementation(AWeapon* TargetWeapon)
	{
		Multi_PickupWeapon(TargetWeapon);
	}

	UFUNCTION(NetMulticast, Reliable)
	void Multi_PickupWeapon(AWeapon* TargetWeapon);
	virtual void Multi_PickupWeapon_Implementation(AWeapon* TargetWeapon);
	
	//Network Reload
	
	UFUNCTION(Server, Reliable)
	void Server_Reload();
	virtual FORCEINLINE void Server_Reload_Implementation()
	{
		Multi_Reload();
	}

	UFUNCTION(NetMulticast, Reliable)
	void Multi_Reload();
	virtual void Multi_Reload_Implementation();
	
	//Network Primary/Secondary Attack
	
	UFUNCTION(Server, Reliable)
	void Server_PrimaryAttack(const bool bAttacking = true);
	virtual FORCEINLINE void Server_PrimaryAttack_Implementation(const bool bAttacking)
	{
		Multi_PrimaryAttack(bAttacking);
	}

	UFUNCTION(NetMulticast, Reliable)
	void Multi_PrimaryAttack(const bool bAttacking);
	virtual void Multi_PrimaryAttack_Implementation(const bool bAttacking);
	
	/*UFUNCTION()
	virtual void OnRep_PrimaryAttacking();*/
	
	/*UFUNCTION()
	virtual void OnRep_SecondaryAttacking();
	*/
	
	/*UFUNCTION(Server, Reliable)
	void Server_PrimaryAttacking();
	virtual void Server_PrimaryAttacking_Implementation();*/

	/*UFUNCTION(Server, Reliable)
	void Server_SecondaryAttacking();
	virtual void Server_SecondaryAttacking_Implementation();*/
	
	UFUNCTION()
	virtual void TimelineProgress(const float Scale);

	UPROPERTY(ReplicatedUsing=OnRep_Health)
	int32 Health;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void NotifyActorBeginOverlap(AActor* OtherActor);
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
	
	UFUNCTION(BlueprintCallable)
	int32 GetHealth() {return Health;}

	UFUNCTION(BlueprintCallable)
	void SetHealth(int32 TargetHealth) {Health = TargetHealth;}
	
	UFUNCTION(BlueprintCallable)
	void Die();
	
	UFUNCTION(BlueprintCallable)
	bool CanPickupWeapon(AWeapon* TargetWeapon);
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	virtual void EquipWeapon(const int32 WeaponIndex);

	UFUNCTION(BlueprintCallable)
	FHitResult GetAimResult(float Distance);

	UFUNCTION(BlueprintCallable)
	FVector GetAimVector();

	UFUNCTION(BlueprintCallable)
	virtual void PickupWeapon(AWeapon* TargetWeapon);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Components")
	class USkeletalMeshComponent* ViewMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentWeapon, Category = "State")
	class AWeapon* CurrentWeapon;
	
	UFUNCTION(BlueprintCallable)
	FORCEINLINE AWeapon* GetCurrentWeapon() const {return CurrentWeapon;}
	
	UPROPERTY(BlueprintAssignable, Category = "Delegates")
	FCurrentWeaponChangeDelegate CurrentWeaponChangeDelegate;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Replicated, Category = "State")
	TArray<class AWeapon*> Weapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
	int32 CurrentWeaponIndex = 0;

	//UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, ReplicatedUsing=OnRep_PrimaryAttacking, Category="State")
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="State")
	bool bPrimaryAttacking = false;

	/*UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, ReplicatedUsing=OnRep_SecondaryAttacking, Category="State")
	bool SecondaryAttacking = false;*/

	UFUNCTION(BlueprintCallable)
	virtual void PrimaryAttack(bool bAttacking);

	UFUNCTION(BlueprintCallable)
	virtual void Reload();
	
	UFUNCTION(BlueprintCallable, Category="Anim")
	virtual void StartAiming();

	UFUNCTION(BlueprintCallable, Category="Anim")
	virtual void EndAiming();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="Anim")
	float ADSWeight = 0.f;
	
};
