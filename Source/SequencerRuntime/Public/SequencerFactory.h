// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SequencerFactory.generated.h"

USTRUCT()
struct FTransformKeyframe
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	FTransform Transform;
	
	UPROPERTY(EditAnywhere)
	double TimeInSeconds;
};

UCLASS()
class SEQUENCERRUNTIME_API ASequencerFactory : public AActor
{
	GENERATED_BODY()
	
public:	
	ASequencerFactory();
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	class ULevelSequence* LevelSequence;

public:	

	UPROPERTY(EditAnywhere)
	AActor* TargetActor;

	UPROPERTY(EditAnywhere)
	TArray<FTransformKeyframe> Keyframes;

	UPROPERTY(EditAnywhere)
	FFrameRate FrameRate = FFrameRate(60,1);

	UPROPERTY(EditAnywhere)
	double SequenceLengthInSeconds = 60;
};
