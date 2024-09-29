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

	switch (KeyframesDataSource)
	{
	case EKeyframesDataSource::Array:
		AddKeyFramesFromArray(MovieScene, TransformSection);
		break;
	case EKeyframesDataSource::SourceLevelSequence:
		AddKeyFramesFromSourceLevelSequence(TransformSection);
		break;
	}

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

FMovieSceneDoubleChannel* ASequencerFactory::GetMovieSceneDoubleChannel(const FMovieSceneChannelProxy& ChannelProxy, const uint32 ChannelIndex)
{
	return ChannelProxy.GetChannel<FMovieSceneDoubleChannel>(ChannelIndex);
}

void ASequencerFactory::AddKeyFramesFromArray(const UMovieScene* MovieScene, const UMovieScene3DTransformSection* TransformSection)
{
	const FMovieSceneChannelProxy& ChannelProxy = TransformSection->GetChannelProxy();

	FMovieSceneDoubleChannel* TranslationXChannel = GetMovieSceneDoubleChannel(ChannelProxy, 0); // X
	FMovieSceneDoubleChannel* TranslationYChannel = GetMovieSceneDoubleChannel(ChannelProxy, 1); // Y
	FMovieSceneDoubleChannel* TranslationZChannel = GetMovieSceneDoubleChannel(ChannelProxy, 2); // Z

	FMovieSceneDoubleChannel* RotationXChannel = GetMovieSceneDoubleChannel(ChannelProxy, 3); // Roll
	FMovieSceneDoubleChannel* RotationYChannel = GetMovieSceneDoubleChannel(ChannelProxy, 4); // Pitch
	FMovieSceneDoubleChannel* RotationZChannel = GetMovieSceneDoubleChannel(ChannelProxy, 5); // Yaw

	FMovieSceneDoubleChannel* ScaleXChannel = GetMovieSceneDoubleChannel(ChannelProxy, 6); // Scale X
	FMovieSceneDoubleChannel* ScaleYChannel = GetMovieSceneDoubleChannel(ChannelProxy, 7); // Scale Y
	FMovieSceneDoubleChannel* ScaleZChannel = GetMovieSceneDoubleChannel(ChannelProxy, 8); // Scale Z

	for (int32 i = 0; i < Keyframes.Num(); ++i)
	{
		auto [Transform, TimeInSeconds, KeyInterpolation] = Keyframes[i];

		const FVector Location = Transform.GetLocation();
		const FRotator Rotation = Transform.GetRotation().Rotator();
		const FVector Scale = Transform.GetScale3D();

		const FFrameNumber FrameNumber = MovieScene->GetTickResolution().AsFrameNumber(TimeInSeconds);
		
		AddKeyFrameToChannel(TranslationXChannel, FrameNumber, Location.X, KeyInterpolation);
		AddKeyFrameToChannel(TranslationYChannel, FrameNumber, Location.Y, KeyInterpolation);
		AddKeyFrameToChannel(TranslationZChannel, FrameNumber, Location.Z, KeyInterpolation);

		AddKeyFrameToChannel(RotationXChannel, FrameNumber, Rotation.Roll, KeyInterpolation);
		AddKeyFrameToChannel(RotationYChannel, FrameNumber, Rotation.Pitch, KeyInterpolation);
		AddKeyFrameToChannel(RotationZChannel, FrameNumber, Rotation.Yaw, KeyInterpolation);

		AddKeyFrameToChannel(ScaleXChannel, FrameNumber, Scale.X, KeyInterpolation);
		AddKeyFrameToChannel(ScaleYChannel, FrameNumber, Scale.Y, KeyInterpolation);
		AddKeyFrameToChannel(ScaleZChannel, FrameNumber, Scale.Z, KeyInterpolation);
	}
}

void ASequencerFactory::AddKeyFrameToChannel(FMovieSceneDoubleChannel* Channel, const FFrameNumber& FrameNumber, const double Value, const EKeyInterpolation KeyInterpolation)
{
	switch (KeyInterpolation) {
	case EKeyInterpolation::Auto:
		AddKeyToChannel(Channel, FrameNumber, Value, EMovieSceneKeyInterpolation::Auto);
		break;
	case EKeyInterpolation::Linear:
		Channel->AddLinearKey(FrameNumber, Value);
		break;
	case EKeyInterpolation::Constant:
		Channel->AddConstantKey(FrameNumber, Value);
		break;
	case EKeyInterpolation::Cubic:
		Channel->AddCubicKey(FrameNumber, Value);
		break;
	}
}

void ASequencerFactory::AddKeyFramesFromSourceLevelSequence(const UMovieScene3DTransformSection* TransformSection) const
{
	const TArray<FMovieSceneBinding>& ObjectBindings = SourceLevelSequence->GetMovieScene()->GetBindings();

	for (const FMovieSceneBinding& Binding : ObjectBindings)
	{
		const TArray<UMovieSceneTrack*>& Tracks = Binding.GetTracks();

		for (UMovieSceneTrack* Track : Tracks)
		{
			const TArray<UMovieSceneSection*>& TransformSections = Cast<UMovieScene3DTransformTrack>(Track)->GetAllSections();
			for (UMovieSceneSection* Section : TransformSections)
			{
				UMovieScene3DTransformSection* SourceTransformSection = Cast<UMovieScene3DTransformSection>(Section);

				const FMovieSceneChannelProxy& SourceChannelProxy = SourceTransformSection->GetChannelProxy();
				FMovieSceneChannelProxy& TargetChannelProxy = TransformSection->GetChannelProxy();

				CopyChannel(SourceChannelProxy, TargetChannelProxy, 0); // X
				CopyChannel(SourceChannelProxy, TargetChannelProxy, 1); // Y
				CopyChannel(SourceChannelProxy, TargetChannelProxy, 2); // Z

				CopyChannel(SourceChannelProxy, TargetChannelProxy, 3); // Roll
				CopyChannel(SourceChannelProxy, TargetChannelProxy, 4); // Pitch
				CopyChannel(SourceChannelProxy, TargetChannelProxy, 5); // Yaw

				CopyChannel(SourceChannelProxy, TargetChannelProxy, 6); // Scale X
				CopyChannel(SourceChannelProxy, TargetChannelProxy, 7); // Scale Y
				CopyChannel(SourceChannelProxy, TargetChannelProxy, 8); // Scale Z
			}
		}
	}
}

void ASequencerFactory::CopyChannel(const FMovieSceneChannelProxy& SourceChannelProxy, const FMovieSceneChannelProxy& TargetChannelProxy, const uint32 ChannelIndex)
{
	FMovieSceneDoubleChannel* SourceChannel = GetMovieSceneDoubleChannel(SourceChannelProxy, ChannelIndex);
	FMovieSceneDoubleChannel* TargetChannel = GetMovieSceneDoubleChannel(TargetChannelProxy, ChannelIndex);

	const TArrayView<const FFrameNumber>& SourceTimes = SourceChannel->GetTimes();
	const TArrayView<const FMovieSceneDoubleValue>& SourceValues = SourceChannel->GetValues();

	for (int32 i = 0; i < SourceTimes.Num(); ++i)
	{
		switch (SourceValues[i].InterpMode)
		{
		case RCIM_Linear:
			TargetChannel->AddLinearKey(SourceTimes[i], SourceValues[i].Value);
			break;
		case RCIM_Constant:
			TargetChannel->AddConstantKey(SourceTimes[i], SourceValues[i].Value);
			break;
		case RCIM_Cubic:
			TargetChannel->AddCubicKey(SourceTimes[i], SourceValues[i].Value);
			break;
		default:
			AddKeyToChannel(TargetChannel, SourceTimes[i], SourceValues[i].Value, EMovieSceneKeyInterpolation::Auto);
			break;
		}
	}
}

void ASequencerFactory::PlaySequence() const
{
	const FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), LevelSequence, PlaybackSettings, SequenceActor);

	SequencePlayer->Play();
}
