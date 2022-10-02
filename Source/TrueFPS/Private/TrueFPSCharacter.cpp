// Fill out your copyright notice in the Description page of Project Settings.

#include "TrueFPSCharacter.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ATrueFPSCharacter::ATrueFPSCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SetCanBeDamaged(true);
	
	GetMesh()->bVisibleInReflectionCaptures = true;
	GetMesh()->bCastHiddenShadow = true;
	GetMesh()->SetTickGroup(ETickingGroup::TG_PostUpdateWork);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);

	//Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	ViewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ClientMesh"));
	ViewMesh->SetCastShadow(false);
	ViewMesh->bCastHiddenShadow = false;
	ViewMesh->bVisibleInReflectionCaptures = false;
	ViewMesh->SetTickGroup(ETickingGroup::TG_PostUpdateWork);
	ViewMesh->SetupAttachment(GetMesh());
	ViewMesh->SetVisibility(false);
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->bUsePawnControlRotation = true;
	Camera->SetupAttachment(GetMesh(), FName("head"));

	Health = 100;
}

void ATrueFPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ATrueFPSCharacter, Weapons, COND_None);
	DOREPLIFETIME_CONDITION(ATrueFPSCharacter, CurrentWeapon, COND_None);
	DOREPLIFETIME_CONDITION(ATrueFPSCharacter, ADSWeight, COND_None);
	DOREPLIFETIME_CONDITION(ATrueFPSCharacter, Health, COND_None);
}

void ATrueFPSCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	DOREPLIFETIME_ACTIVE_OVERRIDE(ATrueFPSCharacter, ADSWeight, ADSWeight >= 1.f || ADSWeight <= 0.f);
}

void ATrueFPSCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	(OutLocation) = GetMesh()->GetSocketLocation(FName("head"));

	if (GetController())
	{
		(OutRotation) = GetController()->GetControlRotation();
		return;
	}
}

void ATrueFPSCharacter::OnRep_Health()
{
	if (Health <= 0)
	{
		Die();
	}
}

// Called when the game starts or when spawned
void ATrueFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Setup ADS Timeline
	if (AimingCurve)
	{
		FOnTimelineFloat TimelineFloat;
		TimelineFloat.BindDynamic(this, &ATrueFPSCharacter::TimelineProgress);

		AimingTimeline.AddInterpFloat(AimingCurve, TimelineFloat);	
	}
	
	//View Mesh Logic
	if (IsLocallyControlled())
	{
		ViewMesh->SetVisibility(true);
		ViewMesh->HideBoneByName(FName("neck_01"), EPhysBodyOp::PBO_None);
		GetMesh()->SetVisibility(false);
	}
	else
	{
		ViewMesh->SetVisibility(false);
	}
	
	// Default Weapons Logic
	
	/*for (const TSubclassOf<AWeapon>& WeaponClass : DefaultWeapons)
	{
		if (!WeaponClass) continue;
		FActorSpawnParameters Params;
		Params.Owner = this;
		AWeapon* SpawnWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, Params);
		
		const int32 WeaponIndex = PickupWeapon(SpawnWeapon);
		
		if (WeaponIndex == CurrentWeaponIndex)
		{
			CurrentWeapon = SpawnWeapon;
			OnRep_CurrentWeapon(nullptr);
		}
	}*/

}

// Called every frame
void ATrueFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimingTimeline.TickTimeline(DeltaTime);
}

void ATrueFPSCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	AWeapon* OverlapWeapon = Cast<AWeapon>(OtherActor);

	if (OverlapWeapon && CanPickupWeapon(OverlapWeapon))
	{
		PickupWeapon(OverlapWeapon);
	}	
}

//Weapon Pickup

void ATrueFPSCharacter::PickupWeapon(AWeapon* TargetWeapon)
{
	if (HasAuthority())
	{
		Multi_PickupWeapon(TargetWeapon);
	}
}

void ATrueFPSCharacter::Multi_PickupWeapon_Implementation(AWeapon* TargetWeapon)
{
	const int32 WeaponIndex = Weapons.Add(TargetWeapon);

	TargetWeapon->SetOwner(this);
	TargetWeapon->Mesh->SetSimulatePhysics(false);
	TargetWeapon->Mesh->SetGenerateOverlapEvents(false);
	TargetWeapon->Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	TargetWeapon->GetRootComponent()->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("root"));
	TargetWeapon->GetRootComponent()->SetVisibility(false);
}

void ATrueFPSCharacter::Die()
{
	if (GetMesh()->GetAnimInstance()->IsAnyMontagePlaying())
	{
		GetMesh()->GetAnimInstance()->StopAllMontages(0.25f);
	}

	GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, TEXT("Ouch"));

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);

	Camera->bUsePawnControlRotation = false;
	bUseControllerRotationYaw = false;
	DisableInput(Cast<APlayerController>(GetController()));
	
}

bool ATrueFPSCharacter::CanPickupWeapon(AWeapon* TargetWeapon)
{
	UClass* TargetWeaponClass = TargetWeapon->GetClass();
	for (const AWeapon* InventoryWeapon : Weapons)
	{
		if (InventoryWeapon)
		{
			if (TargetWeapon == InventoryWeapon)
			{
				return false;
			}
		
			if (InventoryWeapon->GetClass() == TargetWeaponClass)
			{
				return false;
			}
		}
	}
	return true;
}

void ATrueFPSCharacter::PrimaryAttack(bool bAttacking)
{
	if (!CurrentWeapon) return;
	if (bAttacking)
	{
		CurrentWeapon->StartPrimaryAttack();	
	}
	else
	{
		CurrentWeapon->EndPrimaryAttack();
	}
}

void ATrueFPSCharacter::StartPrimaryAttacking()
{
	PrimaryAttack(true); //predict on client
	
	if (!HasAuthority())
	{
		Server_PrimaryAttack(true);  //client tells the server to execute
	}
	else
	{
		Multi_PrimaryAttack(true); //server tell the clients to execute
	}
}

void ATrueFPSCharacter::EndPrimaryAttacking()
{
	PrimaryAttack(false);

	if (!HasAuthority())
	{
		Server_PrimaryAttack(false);
	}
	else
	{
		Multi_PrimaryAttack(false);
	}
}

FHitResult ATrueFPSCharacter::GetAimResult(float Distance)
{
	FHitResult HitResult;

	FVector StartTrace;
	FRotator AimRotator;

	GetActorEyesViewPoint(StartTrace, AimRotator);

	FVector EndTrace = StartTrace + (AimRotator.Vector() * Distance);	
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, TraceParams))
	{
		return HitResult;
	}

	return HitResult;
}

FVector ATrueFPSCharacter::GetAimVector()
{
	FHitResult HitResult = GetAimResult(10000);

	if (HitResult.Time < 1.0f)
	{
		return HitResult.Location;
	}

	return HitResult.TraceEnd;
}

void ATrueFPSCharacter::Multi_PrimaryAttack_Implementation(bool bAttacking)
{
	if (!IsLocallyControlled())
	{
		PrimaryAttack(bAttacking);
	}
}

float ATrueFPSCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority()) return 0.f;
	if (CanBeDamaged() && Health > 0)
	{
		float LastHealth = GetHealth();

		FHitResult HitResult;
		FVector ImpulseDir;

		DamageEvent.GetBestHitInfo(this, DamageCauser, HitResult, ImpulseDir);
		ImpulseDir.Normalize();
		FName BoneName = HitResult.BoneName;

		/*if (HeadHitBox.Compare(BoneName) == 0.0f)
		{
			DamageAmount *= 10;
		}*/

		SetHealth(GetHealth() - DamageAmount);
		
		//	UGameplayStatics::PlaySoundAtLocation(GetWorld(), HurtSound, GetActorLocation(), GetActorRotation(), 1.0f, 1.0f, 0, nullptr, nullptr, this);
		
		return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}
	return 0.0f;
}

void ATrueFPSCharacter::Reload()
{
	if (!CurrentWeapon) return;
	
	CurrentWeapon->StartReload();

	if (HasAuthority())
	{
		Multi_Reload();
	}
	else
	{
		Server_Reload();
	}
}

void ATrueFPSCharacter::Multi_Reload_Implementation()
{
	if (IsLocallyControlled()) return;

	CurrentWeapon->StartReload();
}

void ATrueFPSCharacter::StartAiming()
{
	if (!CurrentWeapon) return;
	AimingTimeline.Play();	

	if (HasAuthority())
	{
		Multi_Aim(true);
	}
	else
	{
		Server_Aim(true);
	}
}

void ATrueFPSCharacter::EndAiming()
{
	if (!CurrentWeapon) return;
	AimingTimeline.Reverse();	

	if (HasAuthority())
	{
		Multi_Aim(false);
	}
	else
	{
		Server_Aim(false);
	}
}

void ATrueFPSCharacter::Multi_Aim_Implementation(const bool bForward)
{
	if (!CurrentWeapon) return;
	if (IsLocallyControlled()) return;
	if (bForward)
	{
		AimingTimeline.Play();
	}
	else
	{
		AimingTimeline.Reverse();
	}
}

void ATrueFPSCharacter::TimelineProgress(const float Scale)
{
	ADSWeight = Scale;
}

void ATrueFPSCharacter::MoveForward(const float scale)
{
	if (scale != 0.0f)
	{
		const FVector& Direction = FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, scale);
	}
}

void ATrueFPSCharacter::MoveSide(const float scale)
{
	if (scale != 0.0f)
	{
		const FVector& Direction = FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, scale);
	}
}

void ATrueFPSCharacter::NextWeapon()
{
	const int32 NextWeaponIndex = Weapons.IsValidIndex(CurrentWeaponIndex + 1) ? CurrentWeaponIndex + 1 : 0;
	EquipWeapon(NextWeaponIndex);
}

void ATrueFPSCharacter::PreviousWeapon()
{
	const int32 NextWeaponIndex = Weapons.IsValidIndex(CurrentWeaponIndex - 1) ? CurrentWeaponIndex - 1 : Weapons.Num() - 1;
	EquipWeapon(NextWeaponIndex);
}

void ATrueFPSCharacter::EquipWeapon(const int32 TargetWeaponIndex)
{
	if (!Weapons.IsValidIndex(TargetWeaponIndex) || CurrentWeapon == Weapons[TargetWeaponIndex]) return;
	
	if (IsLocallyControlled() || HasAuthority())
	{
		CurrentWeaponIndex = TargetWeaponIndex;

		const AWeapon* OldWeapon = CurrentWeapon;
		CurrentWeapon = Weapons[TargetWeaponIndex];
		OnRep_CurrentWeapon(OldWeapon);
	}
	
	if (!HasAuthority())
	{
		Server_SetCurrentWeapon(Weapons[TargetWeaponIndex]);
	}
}

void ATrueFPSCharacter::Server_SetCurrentWeapon_Implementation(AWeapon* NewWeapon)
{
	const AWeapon* OldWeapon = CurrentWeapon;
	CurrentWeapon = NewWeapon;
	OnRep_CurrentWeapon(OldWeapon);
}

void ATrueFPSCharacter::OnRep_CurrentWeapon(const AWeapon* OldWeapon)
{
	if (CurrentWeapon)
	{
		const FTransform& PlacementTransform = CurrentWeapon->PlacementTransform * GetMesh()->GetSocketTransform(TEXT("weaponsocket_r"));
			
		CurrentWeapon->SetActorTransform(PlacementTransform, false, nullptr, ETeleportType::TeleportPhysics);
		CurrentWeapon->GetRootComponent()->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform, FName("weaponsocket_r"));
		CurrentWeapon->GetRootComponent()->SetVisibility(true);
	}
	
	if (OldWeapon)
	{
		OldWeapon->GetRootComponent()->SetVisibility(false);
		OldWeapon->GetRootComponent()->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("root"));
	}

	CurrentWeaponChangeDelegate.Broadcast(CurrentWeapon, OldWeapon);
}

// Called to bind functionality to input
void ATrueFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction(FName("Reload"), IE_Pressed, this, &ATrueFPSCharacter::Reload);
	
	PlayerInputComponent->BindAction(FName("PrimaryAttack"), IE_Pressed, this, &ATrueFPSCharacter::StartPrimaryAttacking);
	PlayerInputComponent->BindAction(FName("PrimaryAttack"), IE_Released, this, &ATrueFPSCharacter::EndPrimaryAttacking);
	
	PlayerInputComponent->BindAction(FName("NextWeapon"), IE_Pressed, this, &ATrueFPSCharacter::NextWeapon);
	PlayerInputComponent->BindAction(FName("PreviousWeapon"), IE_Pressed, this, &ATrueFPSCharacter::PreviousWeapon);

	PlayerInputComponent->BindAction(FName("Aiming"), IE_Pressed, this, &ATrueFPSCharacter::StartAiming);
	PlayerInputComponent->BindAction(FName("Aiming"), IE_Released, this, &ATrueFPSCharacter::EndAiming);
	
	PlayerInputComponent->BindAxis(FName("MoveForward"), this, &ATrueFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis(FName("MoveSide"), this, &ATrueFPSCharacter::MoveSide);

	PlayerInputComponent->BindAxis(FName("LookUp"), this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis(FName("LookSide"), this, &APawn::AddControllerYawInput);

}
