// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 制御点 位置と質量をもつ
struct MassPoint
{
	// コンストラクタ
	MassPoint() = default;
	MassPoint(float invMass, FVector pos, bool isKinematic) {
		initialize(invMass, pos, isKinematic);
	}
	// 初期化
	void initialize(float invMass, FVector pos, bool isKinematic) {
		invMass_ = invMass;
		position_ = prevPosition_ = nextPosition_ = pos;
		velocity_ = FVector(0, 0, 0);
		isKinematic_ = isKinematic;
	}
	// 拘束計算の前処理
	void prepare() {
		nextPosition_ = position_;
	}
	// 拘束計算の適応
	void apply() {
		position_ = nextPosition_;
	}
	// 座標の更新
	void updatePosition(float dt) {
		if (dt <= 0) return;
		if (isKinematic_) return;

		prevPosition_ = position_;
		position_ += velocity_ * dt;
	}
	// 速度の更新
	void updateVelocity(float dt, float gravity) {
		velocity_ = (position_ - prevPosition_) * (1.0f / dt);
		velocity_.Z += gravity * dt;
	}
	// 速度の減衰
	void solveVelocity(float dampCoeff, float dt) {
		float v = velocity_.Length();
		if (v <= 0.0f) return;

		FVector n = velocity_ / v;
		float dv = -v * FMath::Min(1.0f, dampCoeff * dt * getInvMass());
		velocity_ += n * dv;
	}
	// 座標を設定
	void setPosition(FVector pos) {
		position_ = prevPosition_ = pos;
	}
	// 質量の逆数を返す
	float getInvMass() const {
		return isKinematic_ ? 0.0f : invMass_;
	}

	FVector position_ = FVector(0, 0, 0);
	FVector prevPosition_ = FVector(0, 0, 0);
	FVector nextPosition_ = FVector(0, 0, 0);
	FVector velocity_ = FVector(0, 0, 0);
	bool isKinematic_ = false;
	float invMass_ = 0.0f;
};

// 距離拘束
struct DistanceConstraint
{
	virtual ~DistanceConstraint() = default;
	virtual void initLambda() = 0;
	virtual void solvePosition(float dt) = 0;
	virtual void solveVelocity(float dt, float dampCoeff) = 0;

	MassPoint* a_ = nullptr;
	MassPoint* b_ = nullptr;
};

// 距離拘束(PDB)
struct DistanceConstraint_PBD : public DistanceConstraint
{
	DistanceConstraint_PBD() = default;
	DistanceConstraint_PBD(float length, float stiffness, MassPoint* aRef, MassPoint* bRef) {
		initialize(length, stiffness, aRef, bRef);
	}
	void initialize(float length, float stiffness, MassPoint* aRef, MassPoint* bRef) {
		length_ = FMath::Max(length, 0.0f);
		stiffness_ = FMath::Clamp(stiffness, 0.0f, 1.0f);
		a_ = aRef;
		b_ = bRef;
	}
	void initLambda() override {

	}
	void solvePosition(float dt) override {
		float sumMass = a_->getInvMass() + b_->getInvMass();
		if (sumMass <= 0.0f) return;

		FVector v = b_->position_ - a_->position_;
		float d = v.Length();
		if (d <= 0.0f) return;

		float constraint = d - length_;// 目標の距離
		v = v * constraint / (d * sumMass) * stiffness_;

		a_->position_ += v * a_->getInvMass();
		b_->position_ -= v * b_->getInvMass();
	}
	void solveVelocity(float dt, float dampCoeff) override {
		FVector v = b_->position_ - a_->position_;
		float d = v.Length();
		if (d <= 0.0f) return;

		FVector n = v / d;
		auto v0 = FVector::DotProduct(n, a_->velocity_);
		auto v1 = FVector::DotProduct(n, b_->velocity_);
		auto dv0 = (v1 - v0) * FMath::Min(0.5f, dampCoeff * dt * a_->getInvMass());
		auto dv1 = (v0 - v1) * FMath::Min(0.5f, dampCoeff * dt * b_->getInvMass());
		a_->velocity_ += n * dv0;
		b_->velocity_ += n * dv1;
	}

	float length_ = 0.0f;
	float stiffness_ = 0.0f;
};

// 距離拘束(XPDB)
struct DistanceConstraint_XPBD : public DistanceConstraint
{
	DistanceConstraint_XPBD() = default;
	DistanceConstraint_XPBD(float length, float compliance, MassPoint* aRef, MassPoint* bRef) {
		initialize(length, compliance, aRef, bRef);
	}
	void initialize(float length, float compliance, MassPoint* aRef, MassPoint* bRef) {
		length_ = FMath::Max(length, 0.0f);
		compliance_ = FMath::Max(compliance, 0.0f);
		a_ = aRef;
		b_ = bRef;
	}
	void initLambda() override {
		lambda_ = 0.0f;
	}
	void solvePosition(float dt) override {
		float sumMass = a_->getInvMass() + b_->getInvMass();
		if (sumMass <= 0.0f) { return; }

		FVector v = b_->position_ - a_->position_;
		float d = v.Length();
		if (d <= 0.0f) { return; }

		float constraint = d - length_;// 目標の距離
		float compliance = compliance_ / (dt * dt);// コンプライアンス値にdtを加味する
		float dLambda = (constraint - compliance * lambda_) / (sumMass + compliance);// 今回近づける量

		v = (v / d) * dLambda;// 拘束ベクトル
		lambda_ += dLambda;// ラムダを累積

		a_->position_ += v * a_->getInvMass();
		b_->position_ -= v * b_->getInvMass();
	}
	void solveVelocity(float dt, float dampCoeff) override {
		FVector v = b_->position_ - a_->position_;
		float d = v.Length();
		if (d <= 0.0f) return;

		FVector n = v / d;
		auto v0 = FVector::DotProduct(n, a_->velocity_);
		auto v1 = FVector::DotProduct(n, b_->velocity_);
		auto dv0 = (v1 - v0) * FMath::Min(0.5f, dampCoeff * dt * a_->getInvMass());
		auto dv1 = (v0 - v1) * FMath::Min(0.5f, dampCoeff * dt * b_->getInvMass());
		a_->velocity_ += n * dv0;
		b_->velocity_ += n * dv1;
	}

	float length_ = 0.0f;
	float compliance_ = 0.0f;
	float lambda_ = 0.0f;
};
