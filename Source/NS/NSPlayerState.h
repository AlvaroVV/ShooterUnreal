// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerState.h"
#include "NSGameMode.h"
#include "NSPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class NS_API ANSPlayerState : public APlayerState
{
	GENERATED_BODY()

	ANSPlayerState();

public:
	/** Valor que almacena la salud del jugador */ 
	UPROPERTY(Replicated) 
	float Health; 
	
	/** Valor que almacena el número de veces que ha muerto en la partida*/ 
	UPROPERTY(Replicated) 
	uint8 Deaths; 
	
	/** Valor que almacena el equipo al que pertence el jugador */ 
	UPROPERTY(Replicated) 
	ETeam Team;
	
	
};
