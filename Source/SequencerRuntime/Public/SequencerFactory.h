// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieScene.h"
#include "GameFramework/Actor.h"
#include "Sections/MovieScene3DTransformSection.h"
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

	void CreateSequence(UMovieScene*& MovieScene);
	void BindObjectToSequence(UMovieScene* MovieScene, FGuid& BindingID);
	static void AddTransformTrack(UMovieScene* MovieScene, FGuid BindingID, UMovieScene3DTransformSection*& TransformSection);
	static FMovieSceneDoubleChannel* GetMovieSceneDoubleChannel(const FMovieSceneChannelProxy& ChannelProxy, uint32 ChannelIndex);
	void AddKeyFrames(const UMovieScene* MovieScene, const UMovieScene3DTransformSection* TransformSection);
	void PlaySequence() const;
	
	UPROPERTY(VisibleAnywhere)
	class ULevelSequence* LevelSequence = nullptr;

public:	
	UPROPERTY(EditAnywhere)
	AActor* TargetActor = nullptr;

	UPROPERTY(EditAnywhere)
	TArray<FTransformKeyframe> Keyframes;

	UPROPERTY(EditAnywhere)
	FFrameRate FrameRate = FFrameRate(60,1);

	UPROPERTY(EditAnywhere)
	double SequenceLengthInSeconds = 60;
};
