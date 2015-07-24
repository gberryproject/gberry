/* This file is part of GBerry.
 *
 * Copyright 2015 Tero Vuorela
 *
 * GBerry is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * GBerry is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with GBerry. If not, see <http://www.gnu.org/licenses/>.
 */
 
 #include "exitapplicationcommand.h"

namespace GBerryComms {

ExitApplicationCommand::ExitApplicationCommand(const QString& applicationId):
    ICommand("ExitApplication"),
    _applicationId(applicationId)
{
}

ExitApplicationCommand::~ExitApplicationCommand()
{
}

bool ExitApplicationCommand::process(const QJsonObject &json, ICommandResponse& response)
{
    // no response message
    Q_UNUSED(response);
    Q_UNUSED(json);
    emit exitApplicationRequested(_applicationId);
    return true;
}

} // eon
