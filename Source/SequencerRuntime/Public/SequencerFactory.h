// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SequencerFactory.generated.h"

class AActor;
class UMovieScene;
class ULevelSequence;
class UMovieScene3DTransformSection;
struct FMovieSceneChannelProxy;
struct FMovieSceneDoubleChannel;
struct FFrameNumber;

UENUM()
enum class EKeyInterpolation : uint8
{
	Auto,
	Linear,
	Constant,
	Cubic
};

USTRUCT()
struct FTransformKeyframe
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FTransform Transform;
	
	UPROPERTY(EditAnywhere)
	double TimeInSeconds;

	UPROPERTY(EditAnywhere)
	EKeyInterpolation KeyInterpolation;
};

UENUM()
enum class EKeyframesDataSource : uint8
{
	Array,
	SourceLevelSequence
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
	
	void AddKeyFramesFromArray(const UMovieScene* MovieScene, const UMovieScene3DTransformSection* TransformSection);
	static void AddKeyFrameToChannel(FMovieSceneDoubleChannel* Channel, const FFrameNumber& FrameNumber, double Value, EKeyInterpolation KeyInterpolation);

	void AddKeyFramesFromSourceLevelSequence(const UMovieScene3DTransformSection* TransformSection) const;
	static void CopyChannel(const FMovieSceneChannelProxy& SourceChannelProxy, const FMovieSceneChannelProxy& TargetChannelProxy, uint32 ChannelIndex);
	
	void PlaySequence() const;
	
	UPROPERTY(VisibleAnywhere)
	ULevelSequence* LevelSequence = nullptr;

public:	
	UPROPERTY(EditAnywhere)
	AActor* TargetActor = nullptr;

	UPROPERTY(EditAnywhere)
	TArray<FTransformKeyframe> Keyframes;

	UPROPERTY(EditAnywhere)
	FFrameRate FrameRate = FFrameRate(60,1);

	UPROPERTY(EditAnywhere)
	double SequenceLengthInSeconds = 60;

	UPROPERTY(EditAnywhere)
	ULevelSequence* SourceLevelSequence = nullptr;

	UPROPERTY(EditAnywhere)
	EKeyframesDataSource KeyframesDataSource = EKeyframesDataSource::Array;
};
