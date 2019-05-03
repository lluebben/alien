#include "SimulationGpuTest.h"

class TokenSimulationGpuTest
	: public SimulationGpuTest
{
public:
	virtual ~TokenSimulationGpuTest() = default;
};

/**
* Situation: - one horizontal cluster with 10 cells and ascending branch numbers
*			 - first cell has a token
*			 - simulating 9 time steps
* Expected result: token should be on the last cell
*/
TEST_F(TokenSimulationGpuTest, testTokenMovementWithFittingBranchNumbers)
{
	DataDescription origData;
	auto const& cellMaxTokenBranchNumber = _parameters.cellMaxTokenBranchNumber;

	auto cluster = createHorizontalCluster(10, QVector2D{}, QVector2D{}, 0);
	for (int i = 0; i < 10; ++i) {
		auto& cell = cluster.cells->at(i);
		cell.tokenBranchNumber = 1 + i % cellMaxTokenBranchNumber;
	}
	auto& firstCell = cluster.cells->at(0);
	firstCell.addToken(createSimpleToken());
	origData.addCluster(cluster);
	
	uint64_t lastCellId = cluster.cells->at(9).id;

	IntegrationTestHelper::updateData(_access, origData);
	IntegrationTestHelper::runSimulation(9, _controller);

	DataDescription newData = IntegrationTestHelper::getContent(_access, { { 0, 0 },{ _universeSize.x, _universeSize.y } });

	ASSERT_EQ(1, newData.clusters->size());
	auto newCluster = newData.clusters->at(0);

	EXPECT_EQ(10, newCluster.cells->size());

	for (auto const& newCell : *newCluster.cells) {
		if (newCell.id == lastCellId) {
			EXPECT_EQ(1, newCell.tokens->size());
		}
		else if (newCell.tokens) {
			EXPECT_TRUE(newCell.tokens->empty());
		}
	}
}

/**
* Situation: - one horizontal cluster with 10 cells and equal branch numbers
*			 - first cell has a token
*			 - simulating one time step
* Expected result: no token should be on the cells
*/
TEST_F(TokenSimulationGpuTest, testTokenMovementWithUnfittingBranchNumbers)
{
	DataDescription origData;
	auto const& cellMaxTokenBranchNumber = _parameters.cellMaxTokenBranchNumber;

	auto cluster = createHorizontalCluster(10, QVector2D{}, QVector2D{}, 0);
	for (int i = 0; i < 10; ++i) {
		auto& cell = cluster.cells->at(i);
		cell.tokenBranchNumber = 0;
	}
	auto& firstCell = cluster.cells->at(0);
	firstCell.addToken(TokenDescription().setEnergy(30).setData(QByteArray(_parameters.tokenMemorySize, 0)));
	origData.addCluster(cluster);

	IntegrationTestHelper::updateData(_access, origData);
	IntegrationTestHelper::runSimulation(1, _controller);

	DataDescription newData = IntegrationTestHelper::getContent(_access, { { 0, 0 },{ _universeSize.x, _universeSize.y } });

	ASSERT_EQ(1, newData.clusters->size());
	auto newCluster = newData.clusters->at(0);

	EXPECT_EQ(10, newCluster.cells->size());

	for (auto const& newCell : *newCluster.cells) {
		if (newCell.tokens) {
			EXPECT_TRUE(newCell.tokens->empty());
		}
	}
}

/**
* Situation: - one horizontal cluster with 3 cells and branch numbers (1, 0, 1)
*			 - second cell has a token
*			 - simulating one time step
* Expected result: there should be two tokens: on the first and last cell
*/
TEST_F(TokenSimulationGpuTest, testTokenForking)
{
	DataDescription origData;
	auto const& cellMaxTokenBranchNumber = _parameters.cellMaxTokenBranchNumber;

	auto cluster = createHorizontalCluster(3, QVector2D{}, QVector2D{}, 0);
	auto& firstCell = cluster.cells->at(0);
	auto& secondCell = cluster.cells->at(1);
	auto& thirdCell = cluster.cells->at(2);
	firstCell.tokenBranchNumber = 1;
	secondCell.tokenBranchNumber = 0;
	thirdCell.tokenBranchNumber = 1;
	secondCell.addToken(createSimpleToken());
	origData.addCluster(cluster);

	uint64_t firstCellId = firstCell.id;
	uint64_t thirdCellId = thirdCell.id;

	IntegrationTestHelper::updateData(_access, origData);
	IntegrationTestHelper::runSimulation(1, _controller);

	DataDescription newData = IntegrationTestHelper::getContent(_access, { { 0, 0 },{ _universeSize.x, _universeSize.y } });

	ASSERT_EQ(1, newData.clusters->size());
	auto newCluster = newData.clusters->at(0);

	EXPECT_EQ(3, newCluster.cells->size());

	for (auto const& newCell : *newCluster.cells) {
		if (newCell.id == firstCellId || newCell.id == thirdCellId) {
			EXPECT_EQ(1, newCell.tokens->size());
		}
		else if (newCell.tokens) {
			EXPECT_TRUE(newCell.tokens->empty());
		}
	}
}

/**
* Situation: - one horizontal cluster with 5 cells and branch numbers (0, 1, 2, 1, 0)
*			 - first and last cell has a token
*			 - middle cell has too low energy
*			 - simulating 1 time step
* Expected result: cluster decomposes into two cluster, each still has a token
*/
TEST_F(TokenSimulationGpuTest, testTokenMovementDuringDecomposition)
{
	DataDescription origData;
	auto const& cellMaxTokenBranchNumber = _parameters.cellMaxTokenBranchNumber;

	auto lowEnergy = _parameters.cellMinEnergy / 2.0;

	auto cluster = createHorizontalCluster(5, QVector2D{}, QVector2D{}, 0);
	cluster.cells->at(0).tokenBranchNumber = 0;
	cluster.cells->at(1).tokenBranchNumber = 1;
	cluster.cells->at(2).tokenBranchNumber = 2;
	cluster.cells->at(3).tokenBranchNumber = 1;
	cluster.cells->at(4).tokenBranchNumber = 0;
	cluster.cells->at(0).addToken(createSimpleToken());
	cluster.cells->at(4).addToken(createSimpleToken());
	cluster.cells->at(2).energy = lowEnergy;
	origData.addCluster(cluster);

	auto& secondCellId = cluster.cells->at(1).id;
	auto& fourthCellId = cluster.cells->at(3).id;

	IntegrationTestHelper::updateData(_access, origData);
	IntegrationTestHelper::runSimulation(1, _controller);

	DataDescription newData = IntegrationTestHelper::getContent(_access, { { 0, 0 },{ _universeSize.x, _universeSize.y } });

	ASSERT_EQ(2, newData.clusters->size());
	auto clusterById = IntegrationTestHelper::getClusterByCellId(newData);
	{
		auto cluster = clusterById.at(secondCellId);
		EXPECT_EQ(2, cluster.cells->size());
		for (auto const& cell : *cluster.cells) {
			if (cell.id == secondCellId) {
				EXPECT_EQ(1, cell.tokens->size());
			}
			else if (cell.tokens) {
				EXPECT_TRUE(cell.tokens->empty());
			}
		}
	}
	{
		auto cluster = clusterById.at(fourthCellId);
		EXPECT_EQ(2, cluster.cells->size());
		for (auto const& cell : *cluster.cells) {
			if (cell.id == fourthCellId) {
				EXPECT_EQ(1, cell.tokens->size());
			}
			else if (cell.tokens) {
				EXPECT_TRUE(cell.tokens->empty());
			}
		}
	}
}

/**
* Situation: - two horizontal clusters with each 2 cells and branch numbers (0, 1)
*			 - each cluster has token on its first cell
*			 - clusters are colliding for fusion
*			 - simulating 1 time step
* Expected result:
*			 - one cluster with branch numbers (0, 1, 0, 1)
*			 - second cell has two tokens
*			 - fourth cell has one token
*/
TEST_F(TokenSimulationGpuTest, testTokenMovementDuringFusion)
{
	DataDescription origData;
	auto velocity = static_cast<float>(_parameters.cellFusionVelocity) + 0.1f;

	auto firstCluster = createHorizontalCluster(2, QVector2D{ 100, 100 }, QVector2D{ 0, 0 }, 0.0);
	firstCluster.cells->at(0).tokenBranchNumber = 0;
	firstCluster.cells->at(1).tokenBranchNumber = 1;
	firstCluster.cells->at(0).addToken(createSimpleToken());
	setMaxConnections(firstCluster, 2);
	origData.addCluster(firstCluster);

	auto secondCluster = createHorizontalCluster(2, QVector2D{ 102, 100 }, QVector2D{ 0, -velocity }, 0.0);
	secondCluster.cells->at(0).tokenBranchNumber = 0;
	secondCluster.cells->at(1).tokenBranchNumber = 1;
	secondCluster.cells->at(0).addToken(createSimpleToken());
	setMaxConnections(secondCluster, 2);
	origData.addCluster(secondCluster);

	auto secondCellId = firstCluster.cells->at(1).id;
	auto fourthCellId = secondCluster.cells->at(1).id;

	IntegrationTestHelper::updateData(_access, origData);
	IntegrationTestHelper::runSimulation(1, _controller);

	DataDescription newData = IntegrationTestHelper::getContent(_access, { { 0, 0 },{ _universeSize.x, _universeSize.y } });

	ASSERT_EQ(1, newData.clusters->size());
	auto newCluster = newData.clusters->at(0);
	EXPECT_EQ(4, newCluster.cells->size());
	for (auto const& newCell : *newCluster.cells) {
		if (newCell.id == secondCellId) {
			EXPECT_EQ(2, newCell.tokens->size());
		}
		else if (newCell.id == fourthCellId) {
			EXPECT_EQ(1, newCell.tokens->size());
		}
		else if (newCell.tokens) {
			EXPECT_TRUE(newCell.tokens->empty());
		}
	}
}

/**
* Situation: - one horizontal cluster with 3 cells and branch numbers (0, 1, 0)
*			 - first cell has cellMaxToken tokens
*			 - third cell has 1 token
*			 - simulating one time step
* Expected result: second cell should have cellMaxToken tokens
*/
TEST_F(TokenSimulationGpuTest, testTokenMovementWithTooManyTokens)
{
	DataDescription origData;
	auto const& cellMaxTokenBranchNumber = _parameters.cellMaxTokenBranchNumber;

	auto cluster = createHorizontalCluster(3, QVector2D{}, QVector2D{}, 0);
	auto& firstCell = cluster.cells->at(0);
	auto& secondCell = cluster.cells->at(1);
	auto& thirdCell = cluster.cells->at(2);
	firstCell.tokenBranchNumber = 0;
	secondCell.tokenBranchNumber = 1;
	thirdCell.tokenBranchNumber = 0;
	for (int i = 0; i < _parameters.cellMaxToken; ++i) {
		firstCell.addToken(createSimpleToken());
	}
	thirdCell.addToken(createSimpleToken());
	origData.addCluster(cluster);

	uint64_t secondCellId = secondCell.id;

	IntegrationTestHelper::updateData(_access, origData);
	IntegrationTestHelper::runSimulation(1, _controller);

	DataDescription newData = IntegrationTestHelper::getContent(_access, { { 0, 0 },{ _universeSize.x, _universeSize.y } });

	ASSERT_EQ(1, newData.clusters->size());
	auto newCluster = newData.clusters->at(0);

	EXPECT_EQ(3, newCluster.cells->size());

	for (auto const& newCell : *newCluster.cells) {
		if (newCell.id == secondCellId) {
			EXPECT_EQ(_parameters.cellMaxToken, newCell.tokens->size());
		}
		else if (newCell.tokens) {
			EXPECT_TRUE(newCell.tokens->empty());
		}
	}
}
