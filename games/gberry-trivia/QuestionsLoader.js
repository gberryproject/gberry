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

.import GBerry.Log 1.0 as Log
//Log.initLog("AppBoxMaster", gsettings.logLevel)

var _questionData

function loadQuestions() {
    _questionData = Assets.readAll("questions.json")
}

function data() {
    return _questionData
}

function dataJson() {
    try{
        return JSON.parse(_questionData)
    } catch(e) {
        console.error(e.toString()); //error in the above string(in this case,yes)!
        return {}
    }
}
