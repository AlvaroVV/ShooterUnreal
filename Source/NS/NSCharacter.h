// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/Character.h"
#include "NSGameMode.h"
#include "NSCharacter.generated.h"

class UInputComponent;

UCLASS(config=Game)
class ANSCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* FP_Mesh;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** Gun mesh: 3rd person view */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* TP_Gun;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;
public:
	ANSCharacter();

	virtual void BeginPlay();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	/** Sound to indicate player has been hurt*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class USoundBase* PainSound;

	/** AnimMontage to play each time we fire 1st*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FP_FireAnimation;

	/** AnimMontage to play each time we fire 3rd */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* TP_FireAnimation;

	/** ShotParticles in 1st*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay) 
	class UParticleSystemComponent* FP_GunShotParticle;

	/** ShotParticles in 3rd*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay) 
	class UParticleSystemComponent* TP_GunShotParticle;

	/** Bullet Particles */ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay) 
	class UParticleSystemComponent* BulletParticle;

	/** Succeed Shot*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay) 
	class UForceFeedbackEffect* HitSuccessFeedback;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Team)
	ETeam CurrentTeam;

protected:

	/** Material del equipo*/
	class UMaterialInstanceDynamic* DynamicMat;

	/** Estado del jugador */
	class ANSPlayerState* NSPlayerState;
	
	/** Fires a projectile. */
	void OnFire();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/*Método para el servidor que dibuja el rayo*/
	void Fire(const FVector pos, const FVector dir);
	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override; 
	
	virtual void PossessedBy(AController* NewController) override;
	// End of APawn interface


public:
	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return FP_Mesh; }
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/*Conseguir el Player State*/
	class ANSPlayerState* GetNSPlayerState();

	/*Asignar un nuevo Player State*/
	void SetNSPlayerState(class ANSPlayerState* newPS);

	/*Informar para respawnear*/
	void Respawn();

private:

	//FUNCIONES RPC
	/** Informar al servidor de que un jugador ha disparado y el servidor 
	debe comprobar la trayectoria para saber si ha tenido éxito. */ 
	UFUNCTION(Server, Reliable, WithValidation) 
	void ServerFire(const FVector pos, const FVector dir);

	/** Método para informar a todos los clientes conectados los efectos de un disparo. */ 
	UFUNCTION(NetMultiCast, unreliable) 
	void MultiCastShootEffects();

	/** Método para modificar el Mesh de un jugador cuando este ha muerto. */
	UFUNCTION(NetMultiCast, unreliable) 
	void MultiCastRagdoll();

	/** Método llamado en el servidor cuando un jugador ha sufrido daños. */
	UFUNCTION(Client, Reliable) 
	void PlayPain();

public:

	/** Método para modificar a todos los clientes de 
	que pertenecen a un equipo concreto */
	UFUNCTION(NetMultiCast, Reliable) 
	void SetTeam(ETeam NewTeam);
};

