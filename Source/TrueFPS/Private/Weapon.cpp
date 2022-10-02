// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "DrawDebugHelpers.h"
#include "TrueFPSCharacter.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
	SetReplicatingMovement(true);
	SetTickGroup(ETickingGroup::TG_DuringPhysics);
	
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetSimulatePhysics(true);
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetNotifyRigidBodyCollision(true);
	
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	LastFiredTime = 0.0f;
	CurrentState = EWeaponState::IDLE;
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	AmmoCount = Configs.Capacity;
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AWeapon, AmmoCount, COND_None);
	DOREPLIFETIME_CONDITION(AWeapon, CurrentState, COND_SkipOwner);
}

void AWeapon::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	//DOREPLIFETIME_ACTIVE_OVERRIDE(AWeapon, AmmoCount, AmmoCount > Configs.Capacity || AmmoCount < 0.f);
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::StartPrimaryAttack()
{
	if (CanPrimaryAttack())
	{
		HandlePrimaryAttack();
	}
	
	/*float SlackTimeThisFrame = FMath::Max(0.0f, (GetWorld()->TimeSeconds - LastFiredTime) - Configs.Delay);
	PrimaryAttackDelay-= SlackTimeThisFrame;
	
	GetWorldTimerManager().SetTimer(TimeHandle_PrimaryAttack, this, &AWeapon::HandlePrimaryAttack, PrimaryAttackDelay, false);
	PrimaryAttackDelay = Configs.Delay;*/
}

void AWeapon::EndPrimaryAttack()
{
	GetWorldTimerManager().ClearTimer(TimeHandle_PrimaryAttack);

	if(CurrentState == EWeaponState::ATTACKING)
	{
		CurrentState = EWeaponState::IDLE;
	}
}

void AWeapon::StartHolster()
{
	GetWorldTimerManager().ClearTimer(TimeHandle_Reload);
	GetWorldTimerManager().ClearTimer(TimeHandle_PrimaryAttack);
	CurrentState = EWeaponState::IDLE;
}

void AWeapon::EndHolster()
{
}

void AWeapon::CancelHolster()
{
}

void AWeapon::EndReload()
{
	AmmoCount = Configs.Capacity;
	CurrentState = EWeaponState::IDLE;
}

void AWeapon::StartReload()
{
	if (AmmoCount == Configs.Capacity) return;
	if (CurrentState == EWeaponState::ATTACKING || CurrentState == EWeaponState::RELOADING || CurrentState == EWeaponState::DEPLOYING) return;
	if (ReloadMontage && BodyReloadMontage)
	{
		float ReloadMontageTime = Mesh->GetAnimInstance()->Montage_Play(ReloadMontage, 1.0f);
		if(GetOwner())
		{
			Cast<ATrueFPSCharacter>(GetOwner())->GetMesh()->GetAnimInstance()->Montage_Play(BodyReloadMontage, 1.0f);
		}
		CurrentState = EWeaponState::RELOADING;
		GetWorldTimerManager().SetTimer(TimeHandle_Reload, this, &AWeapon::EndReload, ReloadMontageTime, false);
		return;
	}
	EndReload();
}

void AWeapon::HandlePrimaryAttack()
{
	LastFiredTime = GetWorld()->TimeSeconds;
	AmmoCount -= 1;
	ATrueFPSCharacter* Character = Cast<ATrueFPSCharacter>(GetOwner());
	FVector At = Character->GetAimVector();

	FVector OutLocation;
	FRotator OutRotation;
	
	Character->GetActorEyesViewPoint(OutLocation, OutRotation); 
	
	for (int i = 0; i < Configs.NumBullets; i++) {
		FHitResult HitResult;
		//FVector StartTrace = Mesh->GetSocketLocation(FName("MuzzleFlash"));
		FVector StartTrace = OutLocation;
		//FVector ShootDirection = At - StartTrace;
		//FVector ShootDirection =  Mesh->GetSocketRotation(FName("MuzzleFlash")).Vector();
		FVector ShootDirection = OutRotation.Vector();
		FVector SpreadDirection = FMath::VRandCone(ShootDirection, Configs.Spread);
		FVector EndTrace = StartTrace + (SpreadDirection * Configs.Distance);
		FCollisionQueryParams TraceParams;

		TraceParams.AddIgnoredActor(this);
		TraceParams.AddIgnoredActor(GetOwner());

		FVector HitLocation;
		AActor* HitActor;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, TraceParams))
		{
			HitLocation = HitResult.Location;
			HitActor = HitResult.Actor.Get();

			if (HitActor)
			{
				UGameplayStatics::ApplyPointDamage(HitActor, Configs.Damage, SpreadDirection, HitResult, Character->GetController(), this, UDamageType::StaticClass());
			}
		}else{
			HitLocation = EndTrace;
		}

		DrawDebugLine(GetWorld(), StartTrace, HitLocation, FColor::Red, false, 5.0f);
	}
	
	if (AttackMontage)
	{
		Mesh->GetAnimInstance()->Montage_Play(AttackMontage, 1.0f);
	}

	if (GetOwner() && BodyAttackMontage)
	{
		Cast<ATrueFPSCharacter>(GetOwner())->GetMesh()->GetAnimInstance()->Montage_Play(BodyAttackMontage);
	}

	if (AmmoCount > 0 && Configs.Automatic)
	{
		CurrentState = EWeaponState::ATTACKING;
		GetWorldTimerManager().SetTimer(TimeHandle_PrimaryAttack, this, &AWeapon::HandlePrimaryAttack, Configs.Delay, false);	
	}
}

bool AWeapon::CanPrimaryAttack()
{
	if ((LastFiredTime + Configs.Delay) > GetWorld()->TimeSeconds)
	{
		return false;
	}

	if (AmmoCount == 0)
	{
		return false;
	}

	if (CurrentState == EWeaponState::RELOADING || CurrentState == EWeaponState::DEPLOYING)
	{
		return false;
	}
	
	return true;
}
