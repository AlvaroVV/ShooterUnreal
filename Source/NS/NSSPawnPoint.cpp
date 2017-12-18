// Fill out your copyright notice in the Description page of Project Settings.

#include "NS.h"
#include "NSSPawnPoint.h"


// Sets default values
ANSSPawnPoint::ANSSPawnPoint()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	SpawnCapsule->SetCollisionProfileName("OverlapAllDynamic");
	SpawnCapsule->bGenerateOverlapEvents = true;
	SpawnCapsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECollisionResponse::ECR_Overlap);

	OnActorBeginOverlap.AddDynamic(this, &ANSSPawnPoint::ActorBeginOverlaps);

	OnActorEndOverlap.AddDynamic(this, &ANSSPawnPoint::ActorEndOverlaps);
}

void ANSSPawnPoint::OnConstruction(const FTransform& Transform)
{
	if (Team == ETeam::BLUE_TEAM)
	{
		SpawnCapsule->ShapeColor = FColor::Blue;
	}
	else
		SpawnCapsule->ShapeColor = FColor::Red;

}


// Called every frame
void ANSSPawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SpawnCapsule->UpdateOverlaps();
}

void ANSSPawnPoint::ActorBeginOverlaps(AActor* MyOverlappedActor, AActor* OtherActor)
{
	if (Role == ROLE_Authority)
	{
		if (OverlappingActors.Find(OtherActor) == INDEX_NONE)
		{
			OverlappingActors.Add(OtherActor);
		}
	}
}

void ANSSPawnPoint::ActorEndOverlaps(AActor* MyOverlappedActor, AActor* OtherActor)
{
	/**
	* Solo el servidor puede ejecutar estas instrucciones.
	*/
	if (Role == ROLE_Authority)
	{
		if (OverlappingActors.Find(OtherActor) != INDEX_NONE)
		{
			OverlappingActors.Remove(OtherActor);
		}
	}
}
