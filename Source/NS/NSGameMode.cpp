// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NS.h"
#include "NSGameMode.h"
#include "NSHUD.h"
#include "NSPlayerState.h"
#include "NSSPawnPoint.h"
#include "NSCharacter.h"

ANSGameMode::ANSGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	DefaultPawnClass = PlayerPawnClassFinder.Class;
	PlayerStateClass = ANSPlayerState::StaticClass();

	/**
	* TODO - Hace que el atributo GameState se inicialice
	*       con el GameState que hemos creado.
	*/

	// use our custom HUD class
	HUDClass = ANSHUD::StaticClass();

	bReplicates = true;
}

void ANSGameMode::BeginPlay()
{
	Super::BeginPlay();

	/**
	*Comprobar que sólo el servidor puede ejecutar todas las instrucciones
	*  de este método.
	*/
	if (Role == ROLE_Authority)
	{
		for (TActorIterator<ANSSPawnPoint> Iter(GetWorld()); Iter; ++Iter)
		{
			if ((*Iter)->Team == ETeam::RED_TEAM)
			{
				RedSpawns.Add(*Iter);
			}
			else
			{
				BlueSpawns.Add(*Iter);
			}
		}

		// Spawn the server
		APlayerController* thisCont = GetWorld()->GetFirstPlayerController();

		if (thisCont)
		{
			ANSCharacter* thisChar = Cast<ANSCharacter>(thisCont->GetPawn());

			thisChar->SetTeam(ETeam::BLUE_TEAM);
			BlueTeam.Add(thisChar);
			Spawn(thisChar);
		}

		/**
		* TODO - Asignar al atributo creado en el GameState,
		*        el atributo de esta clase que indica si estamos
		*        en el menú o no.
		*/
		//Cast<ANSGameState>(GameState)->????;
	}

}

void ANSGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Quit ||
		EndPlayReason == EEndPlayReason::EndPlayInEditor)
	{
		bInGameMenu = true;
	}
}

void ANSGameMode::Tick(float DeltaSeconds)
{
	/**
	* TODO - Comprobar que sólo el servidor puede ejecutar todas las instrucciones
	*        de este método.
	*/
	if (Role = ROLE_Authority)
	{
		APlayerController* thisCont = GetWorld()->GetFirstPlayerController();

		if (ToBeSpawned.Num() != 0)
		{
			for (auto charToSpawn : ToBeSpawned)
			{
				Spawn(charToSpawn);
			}
		}

		if (thisCont != nullptr && thisCont->IsInputKeyDown(EKeys::R))
		{
			bInGameMenu = false;
			GetWorld()->ServerTravel(L"/Game/FirstPersonCPP/Maps/FirstPersonExampleMap?Listen");

			/**
			* TODO - Asignar al atributo creado en el GameState,
			*        el atributo de esta clase que indica si estamos
			*        en el menú o no.
			*/
			//Cast<ANSGameState>(GameState)->????;
		}
	}
}

void ANSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ANSCharacter* Teamless = Cast<ANSCharacter>(NewPlayer->GetPawn());

	/**
	* Obtener el ANSPlayerState del nuevo jugador que se ha unido.
	*
	*/
	ANSPlayerState* NPlayerState = Teamless->GetNSPlayerState();

	if (Teamless != nullptr && NPlayerState != nullptr)
	{
		Teamless->SetNSPlayerState(NPlayerState);
	}

	/**
	* Comprobar que somos el servidor cuando queramos
	*        generar al jugador en la partida.
	*/

	if (Role== ROLE_Authority && Teamless != nullptr)
	{
		/**
		* Si el equipo azul tiene más jugadores que el equipo rojo
		*        asignaremos al jugador al equipo rojo. Hay que asignar el
		*        equipo al ANSPlayerState.
		*/
		if (RedTeam.Num() <= BlueTeam.Num())
		{
			RedTeam.Add(Teamless);
			NPlayerState->Team = ETeam::RED_TEAM;
		}
		else
		{
			BlueTeam.Add(Teamless);
			NPlayerState->Team = ETeam::BLUE_TEAM;
		}

		// Assign Team and spawn
		Teamless->SetTeam(NPlayerState->Team);
		Spawn(Teamless);
	}
}

void ANSGameMode::Spawn(class ANSCharacter* Character)
{
	/**
	*Comprobar que somos el servidor cuando queramos
	*        generar al jugador en la partida.
	*/

	if (Role == ROLE_Authority)
	{
		// Find Spawn point that is not blocked
		ANSSPawnPoint* thisSpawn = nullptr;
		TArray<ANSSPawnPoint*>* targetTeam = nullptr;

		/**
		* Según el equipo del personaje, lo asignamos a la
		*        cola de generación de personajes del equipo correspondiente.
		*/
		
		if (Character->GetNSPlayerState()->Team == ETeam::BLUE_TEAM)
		{
			targetTeam = &BlueSpawns;
		}
		else
		{
			targetTeam = &RedSpawns;
		}


		//Spawn points 
		for (auto Spawn : (*targetTeam))
		{
			if (!Spawn->GetBlocked())
			{
				// Remove from spawn queue location
				if (ToBeSpawned.Find(Character) != INDEX_NONE)
				{
					ToBeSpawned.Remove(Character);
				}

				// Otherwise set actor location
				Character->SetActorLocation(Spawn->
					GetActorLocation());

				Spawn->UpdateOverlaps();

				return;
			}
		}

		if (ToBeSpawned.Find(Character) == INDEX_NONE)
		{
			ToBeSpawned.Add(Character);
		}
	}

	
}


void ANSGameMode::Respawn(class ANSCharacter* Character)
{
	/**
	* TODO - Comprobar que somos el servidor cuando queramos
	*        regenerar al jugador en la partida.
	*/
	if (Role == ROLE_Authority)
	{
		AController* thisPC = Character->GetController();
		Character->DetachFromControllerPendingDestroy();

		ANSCharacter* newChar = Cast<ANSCharacter>(GetWorld()->SpawnActor(DefaultPawnClass));

		if (newChar)
		{
			thisPC->Possess(newChar);
			ANSPlayerState* thisPS = Cast<ANSPlayerState>(newChar->GetController()->PlayerState);

			/**
			* Asignar el equipo y el ANSPlayerState al nuevo personaje.
			*/
			thisPS->Team = Character->GetNSPlayerState()->Team;		
			newChar->SetNSPlayerState(thisPS);

			Spawn(newChar);
			
			/**
			* Usar la función SetTeam para indicar a todos los jugadores
			*        el equipo al que pertence el nuevo personaje.
			*/
			newChar->SetTeam(thisPS->Team);
		}
	}

}
