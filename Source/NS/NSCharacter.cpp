// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NS.h"
#include "NSCharacter.h"
#include "NSProjectile.h"
#include "NSPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ANSCharacter

ANSCharacter::ANSCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	FP_Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	FP_Mesh->SetOnlyOwnerSee(true);
	FP_Mesh->SetupAttachment(FirstPersonCameraComponent);
	FP_Mesh->bCastDynamicShadow = false;
	FP_Mesh->CastShadow = false;

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->AttachTo(FP_Mesh, TEXT("GripPoint"), EAttachLocation::SnapToTargetIncludingScale,true);

	//Create gun mesh for 3rd person
	TP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TP_GUN"));
	TP_Gun->SetOwnerNoSee(true);
	TP_Gun->AttachTo(GetMesh(), TEXT("hand_rSocket"), EAttachLocation::SnapToTargetIncludingScale, true);

	GetMesh()->SetOwnerNoSee(true);

	//PArticle System for 3d person
	TP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSysTP")); 
	TP_GunShotParticle->bAutoActivate = false; 
	TP_GunShotParticle->AttachTo(TP_Gun); 
	TP_GunShotParticle->SetOwnerNoSee(true);

	//Particle System for 1st person
	FP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSysFP")); 
	FP_GunShotParticle->bAutoActivate = false; 
	FP_GunShotParticle->AttachTo(FP_Gun); 
	FP_GunShotParticle->SetOnlyOwnerSee(true);

	//Bullet Particles
	BulletParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BulletSysTP")); 
	BulletParticle->bAutoActivate = false; 
	BulletParticle->AttachTo(FirstPersonCameraComponent);

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void ANSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//FP_Gun->AttachToComponent(FP_Mesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint")); //Attach gun mesh component to Skeleton, doing it here because the skelton is not yet created in the constructor

	// TODO - Añadir la inicialización del equipo 
	if (Role != ROLE_Authority) 
	{ 
		SetTeam(CurrentTeam); 
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANSCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
    InputComponent->BindAction("Fire", IE_Pressed, this, &ANSCharacter::OnFire);

	InputComponent->BindAxis("MoveForward", this, &ANSCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ANSCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &ANSCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &ANSCharacter::LookUpAtRate);
}

void ANSCharacter::OnFire()
{


	// try and play a firing animation if specified
	if (FP_FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = FP_Mesh->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FP_FireAnimation, 1.f);
		}
	}

	if (FP_GunShotParticle != nullptr)
	{
		FP_GunShotParticle->Activate(true);
	}

	FVector mousePos; 
	FVector mouseDir;

	APlayerController* pController = Cast<APlayerController>(GetController());
	FVector2D ScreenPos = GEngine->GameViewport->Viewport->GetSizeXY();

	pController->DeprojectScreenPositionToWorld(ScreenPos.X / 2.0f, ScreenPos.Y / 2.0f, mousePos, mouseDir);

	mouseDir *= 10000000.0f;

	ServerFire(mousePos, mouseDir);

}

bool ANSCharacter::ServerFire_Validate(const FVector pos, const FVector dir) 
{ 
	// Validamos si la posición y la dirección son válidas. 
	// En este caso, es válido si no son iguales al vector por defecto. 
	if (pos != FVector(ForceInit) && dir != FVector(ForceInit)) 
	{ 
		return true; 
	} 
	else 
	{ 
		return false; 
	} 
}

void ANSCharacter::ServerFire_Implementation(const FVector pos, const FVector dir) 
{ 
	// Llamamos a la función Fire para ejecutar el disparo. 
	Fire(pos, dir); 
	
	// Además, replicamos los efectos del disparo a todos los clientes 
	// con la función definida para ello. 
	MultiCastShootEffects(); 
}

void ANSCharacter::MultiCastShootEffects_Implementation() 
{ 
	// Ejecutamos la animación del disparo en 3ª Persona si está declarada. 
	if (TP_FireAnimation != NULL) 
	{ 
		// Obtenemos la instancia de la animación para el personaje 
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance != NULL) 
		{ 
			AnimInstance->Montage_Play(TP_FireAnimation, 1.f); 
		} 
	} // Ejecutamos el sonido del disparo si está declarado.
	if (FireSound != NULL) 
	{ 
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation()); 
	}

	// Activamos el sistema de particulas del disparo en 3ª persona si está activado. 
	if (TP_GunShotParticle != nullptr) 
	{ 
		TP_GunShotParticle->Activate(true); 
	} 
	// Creamos un proyectil en la localización del disparo. 
	if (BulletParticle != nullptr) 
	{ 
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BulletParticle->Template, BulletParticle->GetComponentLocation(), BulletParticle->GetComponentRotation()); 
	} 
}

void ANSCharacter::Fire(const FVector pos, const FVector dir) 
{ 
	// Representamos el rayo de la trayectoria del proyectil
	FCollisionObjectQueryParams ObjQuery;
	ObjQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1); 

	FCollisionQueryParams ColQuery; 
	ColQuery.AddIgnoredActor(this); 
	FHitResult HitRes; 
	
	GetWorld()->LineTraceSingleByObjectType(HitRes, pos, dir, ObjQuery, ColQuery);

	// Dibujamos una linea que nos permite visualizar la trayectoria. 
	DrawDebugLine(GetWorld(), pos, dir, FColor::Red, true, 100, 0, 5.0f);

	// Preguntamos si el disparo ha impactado en algún objeto. 
	if (HitRes.bBlockingHit) 
	{
		ANSCharacter* OtherChar = Cast<ANSCharacter>(HitRes.GetActor());

		// Preguntamos si el disparo ha impactado en otro jugador y si es del otro equipo al que pertence el personaje. 		
		if (OtherChar != nullptr && OtherChar->GetNSPlayerState()->Team != this->GetNSPlayerState()->Team)
		{ 
			// Indicamos al otro personaje que ha recibido un daño de 10.0f 
			FDamageEvent thisEvent(UDamageType::StaticClass()); 
			OtherChar->TakeDamage(10.0f, thisEvent, this->GetController(), this); 
			
			// Informamos al cliente que tiene el control del personaje, que ha tenido éxito en su disparo.
			APlayerController* thisPC = Cast<APlayerController>(GetController()); 
			thisPC->ClientPlayForceFeedback(HitSuccessFeedback, false, NAME_None); 
		} 
	} 
}

float ANSCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) 
{
	// Llamamos al método de la clase padre 
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser); 
	
	// Comprobamos que esta función está siendo ejecutada por el servidor, 
	// que el daño no se lo esta causando el propio jugador, 
	// y que la salud actual del jugador es mayor que 0. 
	if (Role == ROLE_Authority && DamageCauser != this && NSPlayerState->Health > 0)
	{
		// Restamos la salud, y ejecutamos en el cliente al que pertenece el jugador el sonido de que ha sido dañado. 
		NSPlayerState->Health -= Damage; 
		
		PlayPain(); 
		
		// Comprobamos si ha muerto 
		if (NSPlayerState->Health <= 0) 
		{ 
			// Incrementamos el número de muertes. NSPlayerState->Deaths++;
			// Ejecutamos en todos los clientes la animación de que el personaje a muerto. 
			MultiCastRagdoll();

			// Incrementamos la puntuación del jugador que ha conseguido matar al personaje. 
			ANSCharacter * OtherChar = Cast< ANSCharacter >(DamageCauser); 
			if (OtherChar) 
			{ 
				OtherChar->NSPlayerState->Score += 1.0f; 
			} 
			// Después de 3 segundos, volvemos a crear al jugador en la partida. 
			FTimerHandle thisTimer; 
			GetWorldTimerManager().SetTimer<ANSCharacter>(thisTimer, this, &ANSCharacter::Respawn, 3.0f, false); 
		} 
	} 
	return Damage; 
}

void ANSCharacter::PlayPain_Implementation() 
{ 
	// Ejecutamos el sonido solo si se esta ejecutando en el cliente al que pertenece el jugador. 
	if (Role == ROLE_AutonomousProxy) 
	{ 
		UGameplayStatics::PlaySoundAtLocation(this, PainSound, GetActorLocation()); 
	} 
}

void ANSCharacter::MultiCastRagdoll_Implementation() 
{ 
	GetMesh()->SetPhysicsBlendWeight(1.0f); 
	GetMesh()->SetSimulatePhysics(true); 
	GetMesh()->SetCollisionProfileName("Ragdoll"); 
}

void ANSCharacter::Respawn() 
{ 
	// Comprobamos que esta función está siendo ejecutada por el servidor. 
	if (Role == ROLE_Authority) 
	{ // Restauramos la vida 
		NSPlayerState->Health = 100.0f; 
	} 
}

void ANSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ANSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ANSCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ANSCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ANSCharacter::SetNSPlayerState(ANSPlayerState* newPS) 
{ 
	// Comprobamos que somos el servidor y que el nuevo estado es válido. 
	if (newPS && Role == ROLE_Authority) 
	{ 
		NSPlayerState = newPS; 
		PlayerState = newPS; 
	}
}

ANSPlayerState* ANSCharacter::GetNSPlayerState() 
{ 
	if (NSPlayerState)
	{
		return NSPlayerState;
	}
	else 
	{ 
		NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		return NSPlayerState; 
	}

}


void ANSCharacter::PossessedBy(AController* NewController) 
{ 
	// Llamamos al mismo método de la clase padre. 
	Super::PossessedBy(NewController); 
	
	// Obtenemos el estado del jugador
	NSPlayerState = Cast<ANSPlayerState>(PlayerState); 
	// Si somos el servidor, y el estado del jugador existe, restauramos la saludo a 100.0f. 
	if (Role == ROLE_Authority && NSPlayerState != nullptr)
	{ 
		NSPlayerState->Health = 100.0f; 
	}
}

void ANSCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const 
{ 
	Super::GetLifetimeReplicatedProps(OutLifetimeProps); 
	
	DOREPLIFETIME(ANSCharacter, CurrentTeam);
}

void ANSCharacter::SetTeam_Implementation(ETeam NewTeam) 
{ 
	FLinearColor outColour; 
	// Dependiendo del equipo que seamos, asignamos un color u otro. 

	if (NewTeam == ETeam::BLUE_TEAM) 
	{ 
		outColour = FLinearColor(0.0f, 0.0f, 0.5f); 
	} 
	else 
	{ 
		outColour = FLinearColor(0.5f, 0.0f, 0.0f); 
	} 
	// Asignamos el color al material dinámico y se lo asignamos a los Mesh.
	if (DynamicMat == nullptr)
	{
		DynamicMat = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this); 
		DynamicMat->SetVectorParameterValue(TEXT("BodyColor"), outColour); 

		GetMesh()->SetMaterial(0, DynamicMat); 
		FP_Mesh->SetMaterial(0, DynamicMat);
	}
}