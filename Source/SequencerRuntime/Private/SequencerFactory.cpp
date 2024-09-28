#include "SequencerFactory.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Tracks/MovieScene3DTransformTrack.h"


ASequencerFactory::ASequencerFactory()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASequencerFactory::BeginPlay()
{
	Super::BeginPlay();
	
	LevelSequence = NewObject<ULevelSequence>(this, ULevelSequence::StaticClass());
	LevelSequence->Initialize();
	
	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	MovieScene->SetDisplayRate(FrameRate);

	const FFrameRate TickResolution = MovieScene->GetTickResolution();
	const uint32 Duration = TickResolution.AsFrameNumber(SequenceLengthInSeconds).Value;
	MovieScene->SetPlaybackRange(FFrameNumber(0), Duration);
	
	if (!TargetActor)
	{
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		TargetActor = PlayerController->GetPawn();
		TargetActor->DisableInput(PlayerController);
	}
	
    const FGuid BindingID = MovieScene->AddPossessable(TargetActor->GetName(), TargetActor->GetClass());
	LevelSequence->BindPossessableObject(BindingID, *TargetActor, TargetActor->GetWorld());
	
	UMovieScene3DTransformTrack* TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(BindingID);
	UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());

	TransformSection->SetRange(MovieScene->GetPlaybackRange());
	TransformTrack->AddSection(*TransformSection);
	
	FMovieSceneDoubleChannel* TranslationXChannel = TransformSection->GetChannelProxy().GetChannel<FMovieSceneDoubleChannel>(0);
	FMovieSceneDoubleChannel* TranslationYChannel = TransformSection->GetChannelProxy().GetChannel<FMovieSceneDoubleChannel>(1);
	FMovieSceneDoubleChannel* TranslationZChannel = TransformSection->GetChannelProxy().GetChannel<FMovieSceneDoubleChannel>(2);

	FMovieSceneDoubleChannel* RotationXChannel = TransformSection->GetChannelProxy().GetChannel<FMovieSceneDoubleChannel>(3);
	FMovieSceneDoubleChannel* RotationYChannel = TransformSection->GetChannelProxy().GetChannel<FMovieSceneDoubleChannel>(4);
	FMovieSceneDoubleChannel* RotationZChannel = TransformSection->GetChannelProxy().GetChannel<FMovieSceneDoubleChannel>(5);
	
	for (int32 i = 0; i < Keyframes.Num(); ++i)
	{
		auto [Transform, TimeInSeconds] = Keyframes[i];
		
		const FVector Location = Transform.GetLocation();
		const FRotator Rotation = Transform.GetRotation().Rotator();

		const FFrameNumber FrameNumber = TickResolution.AsFrameNumber(TimeInSeconds);
		
		AddKeyToChannel(TranslationXChannel, FrameNumber, Location.X, EMovieSceneKeyInterpolation::Auto);
		AddKeyToChannel(TranslationYChannel, FrameNumber, Location.Y, EMovieSceneKeyInterpolation::Auto);
		AddKeyToChannel(TranslationZChannel, FrameNumber, Location.Z, EMovieSceneKeyInterpolation::Auto);

		AddKeyToChannel(RotationXChannel, FrameNumber, Rotation.Roll, EMovieSceneKeyInterpolation::Auto);
		AddKeyToChannel(RotationYChannel, FrameNumber, Rotation.Pitch, EMovieSceneKeyInterpolation::Auto);
		AddKeyToChannel(RotationZChannel, FrameNumber, Rotation.Yaw, EMovieSceneKeyInterpolation::Auto);
	}
	
	const FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), LevelSequence, PlaybackSettings, SequenceActor);
	
	SequencePlayer->Play();
}
