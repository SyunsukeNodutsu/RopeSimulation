// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Rope.h"
#include "GameFramework/Actor.h"
#include "PBDRopeActor.generated.h"

/**
 * ロープシミュレーション
 */
UCLASS()
class ROPESIMULATION_API APBDRopeActor : public AActor
{
	GENERATED_BODY()

public:
	// 拘束計算の反復回数
	UPROPERTY(EditAnywhere, Category = "Control")
	int step_ = 10;
	// 速度の減衰係数
	UPROPERTY(EditAnywhere, Category = "Control")
	float globalDampingCoeff_ = 0.0f;
	// エッジの速度の減衰係数
	UPROPERTY(EditAnywhere, Category = "Control")
	float edgeDampingCoeff_ = 0.0f;
	// 質点(制御点)の数
	UPROPERTY(EditAnywhere, Category = "Control")
	int pointNum_ = 14;
	// 重力
	UPROPERTY(EditAnywhere, Category = "Control")
	float gravity_ = 9.8f * 2.0f;
	// 拘束の長さ
	UPROPERTY(EditAnywhere, Category = "Control")
	float length_ = 1.0f;
	// 拘束の固さ0から1の範囲、1で最も固くなる
	UPROPERTY(EditAnywhere, Category = "Control")
	float stiffness_ = 1.0f;
	// コンプライアンス値(剛性の逆数)、0に近いほど固い
	UPROPERTY(EditAnywhere, Category = "Control")
	float compliance_ = 0.0001f;
	// 質点の質量
	UPROPERTY(EditAnywhere, Category = "Control")
	float mass_ = 1.0f;
	// PBDかXPBDか
	UPROPERTY(EditAnywhere, Category = "Control")
	bool isPBD_ = true;
	// プレイヤーと終点を接続するか
	UPROPERTY(EditAnywhere, Category = "Control")
	bool isConnectPlayer_ = true;

	// クリップボードにパラメーターをコピー
	UFUNCTION(CallInEditor, Category = "Control")
	void CopyRopeParametersToClipboard() const;

public:
	APBDRopeActor();
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	void Simulate(float dt, int iter);

private:
	TArray<MassPoint> massPoints_;
	TArray<TUniquePtr<DistanceConstraint>> constraints_;

};
