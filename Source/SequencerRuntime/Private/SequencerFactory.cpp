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

	UMovieScene* MovieScene;
	CreateSequence(MovieScene);

	FGuid BindingID;
	BindObjectToSequence(MovieScene, BindingID);

	UMovieScene3DTransformSection* TransformSection;
	AddTransformTrack(MovieScene, BindingID, TransformSection);

	AddKeyFrames(MovieScene, TransformSection);

	PlaySequence();
}

void ASequencerFactory::CreateSequence(UMovieScene*& MovieScene)
{
	LevelSequence = NewObject<ULevelSequence>(this, ULevelSequence::StaticClass());
	LevelSequence->Initialize();

	MovieScene = LevelSequence->GetMovieScene();
	MovieScene->SetDisplayRate(FrameRate);

	const uint32 Duration = MovieScene->GetTickResolution().AsFrameNumber(SequenceLengthInSeconds).Value;
	MovieScene->SetPlaybackRange(FFrameNumber(0), Duration);
}

void ASequencerFactory::BindObjectToSequence(UMovieScene* MovieScene, FGuid& BindingID)
{
	if (!TargetActor)
	{
		// Bind default pawn if TargetActor is not set
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		TargetActor = PlayerController->GetPawn();
		TargetActor->DisableInput(PlayerController);
	}

	BindingID = MovieScene->AddPossessable(TargetActor->GetName(), TargetActor->GetClass());
	LevelSequence->BindPossessableObject(BindingID, *TargetActor, TargetActor->GetWorld());
}

void ASequencerFactory::AddTransformTrack(UMovieScene* MovieScene, const FGuid BindingID, UMovieScene3DTransformSection*& TransformSection)
{
	UMovieScene3DTransformTrack* TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(BindingID);
	TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());
	TransformSection->SetRange(MovieScene->GetPlaybackRange());
	TransformTrack->AddSection(*TransformSection);
}

FMovieSceneDoubleChannel* ASequencerFactory::GetMovieSceneDoubleChannel(const FMovieSceneChannelProxy& ChannelProxy, uint32 ChannelIndex)
{
	return ChannelProxy.GetChannel<FMovieSceneDoubleChannel>(ChannelIndex);
}

void ASequencerFactory::AddKeyFrames(const UMovieScene* MovieScene, const UMovieScene3DTransformSection* TransformSection)
{
	const FMovieSceneChannelProxy& ChannelProxy = TransformSection->GetChannelProxy();

	FMovieSceneDoubleChannel* TranslationXChannel = GetMovieSceneDoubleChannel(ChannelProxy, 0);
	FMovieSceneDoubleChannel* TranslationYChannel = GetMovieSceneDoubleChannel(ChannelProxy, 1);
	FMovieSceneDoubleChannel* TranslationZChannel = GetMovieSceneDoubleChannel(ChannelProxy, 2);

	FMovieSceneDoubleChannel* RotationXChannel = GetMovieSceneDoubleChannel(ChannelProxy, 3);
	FMovieSceneDoubleChannel* RotationYChannel = GetMovieSceneDoubleChannel(ChannelProxy, 4);
	FMovieSceneDoubleChannel* RotationZChannel = GetMovieSceneDoubleChannel(ChannelProxy, 5);

	for (int32 i = 0; i < Keyframes.Num(); ++i)
	{
		auto [Transform, TimeInSeconds] = Keyframes[i];

		const FVector Location = Transform.GetLocation();
		const FRotator Rotation = Transform.GetRotation().Rotator();

		const FFrameNumber FrameNumber = MovieScene->GetTickResolution().AsFrameNumber(TimeInSeconds);

		AddKeyToChannel(TranslationXChannel, FrameNumber, Location.X, EMovieSceneKeyInterpolation::Auto);
		AddKeyToChannel(TranslationYChannel, FrameNumber, Location.Y, EMovieSceneKeyInterpolation::Auto);
		AddKeyToChannel(TranslationZChannel, FrameNumber, Location.Z, EMovieSceneKeyInterpolation::Auto);

		AddKeyToChannel(RotationXChannel, FrameNumber, Rotation.Roll, EMovieSceneKeyInterpolation::Auto);
		AddKeyToChannel(RotationYChannel, FrameNumber, Rotation.Pitch, EMovieSceneKeyInterpolation::Auto);
		AddKeyToChannel(RotationZChannel, FrameNumber, Rotation.Yaw, EMovieSceneKeyInterpolation::Auto);
	}
}

void ASequencerFactory::PlaySequence() const
{
	const FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), LevelSequence, PlaybackSettings, SequenceActor);

	SequencePlayer->Play();
}
