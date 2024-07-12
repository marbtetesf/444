/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
* Module providing metrics for the EVM optimizer.
*/

#include <libyul/backends/evm/EVMMetrics.h>

#include <libyul/AST.h>
#include <libyul/Exceptions.h>
#include <libyul/Utilities.h>
#include <libyul/backends/evm/EVMDialect.h>

#include <libevmasm/Instruction.h>
#include <libevmasm/GasMeter.h>

#include <libsolutil/CommonData.h>

using namespace solidity;
using namespace solidity::yul;
using namespace solidity::util;

GasMeter::GasMeter(YulNameRepository const& _nameRepository, bool _isCreation, bigint _runs):
	m_nameRepository(_nameRepository),
	m_isCreation{_isCreation},
	m_runs(_isCreation? 1 : _runs)
{
	yulAssert(_nameRepository.isEvmDialect());
}

bigint GasMeter::costs(Expression const& _expression) const
{
	return combineCosts(GasMeterVisitor::costs(_expression, m_nameRepository, m_isCreation));
}

bigint GasMeter::instructionCosts(evmasm::Instruction _instruction) const
{
	return combineCosts(GasMeterVisitor::instructionCosts(_instruction, m_nameRepository, m_isCreation));
}

bigint GasMeter::combineCosts(std::pair<bigint, bigint> _costs) const
{
	return _costs.first * m_runs + _costs.second;
}


std::pair<bigint, bigint> GasMeterVisitor::costs(
	Expression const& _expression,
	YulNameRepository const& _nameRepository,
	bool _isCreation
)
{
	GasMeterVisitor gmv(_nameRepository, _isCreation);
	gmv.visit(_expression);
	return {gmv.m_runGas, gmv.m_dataGas};
}

std::pair<bigint, bigint> GasMeterVisitor::instructionCosts(
	evmasm::Instruction _instruction,
	YulNameRepository const& _nameRepository,
	bool _isCreation
)
{
	GasMeterVisitor gmv(_nameRepository, _isCreation);
	gmv.instructionCostsInternal(_instruction);
	return {gmv.m_runGas, gmv.m_dataGas};
}

GasMeterVisitor::GasMeterVisitor(YulNameRepository const& _nameRepository, bool _isCreation):
	m_nameRepository(_nameRepository),
	m_isCreation{_isCreation}
{
	yulAssert(_nameRepository.isEvmDialect());
}

void GasMeterVisitor::operator()(FunctionCall const& _funCall)
{
	ASTWalker::operator()(_funCall);
	if (BuiltinFunctionForEVM const* f = m_nameRepository.evmDialect()->builtin(_funCall.functionName.name))
		if (f->instruction)
		{
			instructionCostsInternal(*f->instruction);
			return;
		}
	yulAssert(false, "Functions not implemented.");
}

void GasMeterVisitor::operator()(Literal const& _lit)
{
	m_runGas += evmasm::GasMeter::runGas(evmasm::Instruction::PUSH1, m_nameRepository.evmDialect()->evmVersion());
	m_dataGas +=
		singleByteDataGas() +
		evmasm::GasMeter::dataGas(
			toCompactBigEndian(_lit.value.value(), 1),
			m_isCreation,
			m_nameRepository.evmDialect()->evmVersion()
		);
}

void GasMeterVisitor::operator()(Identifier const&)
{
	m_runGas += evmasm::GasMeter::runGas(evmasm::Instruction::DUP1, m_nameRepository.evmDialect()->evmVersion());
	m_dataGas += singleByteDataGas();
}

bigint GasMeterVisitor::singleByteDataGas() const
{
	if (m_isCreation)
		return evmasm::GasCosts::txDataNonZeroGas(m_nameRepository.evmDialect()->evmVersion());
	else
		return evmasm::GasCosts::createDataGas;
}

void GasMeterVisitor::instructionCostsInternal(evmasm::Instruction _instruction)
{
	if (_instruction == evmasm::Instruction::EXP)
		m_runGas += evmasm::GasCosts::expGas + evmasm::GasCosts::expByteGas(m_nameRepository.evmDialect()->evmVersion());
	else if (_instruction == evmasm::Instruction::KECCAK256)
		// Assumes that Keccak-256 is computed on a single word (rounded up).
		m_runGas += evmasm::GasCosts::keccak256Gas + evmasm::GasCosts::keccak256WordGas;
	else
		m_runGas += evmasm::GasMeter::runGas(_instruction, m_nameRepository.evmDialect()->evmVersion());
	m_dataGas += singleByteDataGas();
}
