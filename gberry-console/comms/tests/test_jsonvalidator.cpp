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
 
 #include <gtest/gtest.h>

#include "json/jsonvalidator.h"
#include "json/jsondefinitionbuilder.h"

#include "testutils/qtgtest.h"
#include "testutils/util_enablelog.h"

namespace {
    static TestUtils::StdoutLoggingEnabler& logging = TestUtils::enabledStdoutLogging();
}

TEST(JsonValidator, StringMemberValidJson)
{
    JsonDefinitionBuilder builder;
    builder.hasStringMember("id");
    builder.hasStringMember("description");
    QSharedPointer<JsonDefinition> def = builder.definition(); // using pointer to delete def in the test end

    QJsonObject json;
    json["id"] = "test";
    json["description"] = "test description";
    json["extra"] = "extra member"; // additional not required

    JsonValidator validator(def);
    bool result = validator.validate(json);
    EXPECT_TRUE(result) << validator.errors();
    EXPECT_EQ(validator.errors().length(), 0);
}

TEST(JsonValidator, InvalidJsonMissingMember)
{
    JsonDefinitionBuilder builder;
    builder.hasStringMember("id");
    builder.hasStringMember("description");
    QSharedPointer<JsonDefinition> def = builder.definition();

    QJsonObject json;
    json["not_id"] = "test";
    json["description"] = "test description";
    DEBUG("isString: " << json["description"].isString());
    DEBUG("Type: " << json["description"].type());

    JsonValidator validator(def);
    bool result = validator.validate(json);
    DEBUG("Errors: " << validator.errors());
    EXPECT_FALSE(result) << validator.errors();
    EXPECT_EQ(validator.errors().length(), 1);
}

TEST(JsonValidator, InvalidJsonNotStringMember)
{
    JsonDefinitionBuilder builder;
    builder.hasStringMember("id");
    builder.hasStringMember("description");
    QSharedPointer<JsonDefinition> def = builder.definition();

    QJsonObject json;
    json["id"] = 1;
    json["description"] = "test description";

    JsonValidator validator(def);
    bool result = validator.validate(json);
    EXPECT_FALSE(result) << validator.errors();
    EXPECT_EQ(validator.errors().length(), 1);
}

TEST(JsonValidator, OptionalStringMember)
{
    // -- missing
    JsonDefinitionBuilder builder;
    builder.hasOptionalStringMember("valid");
    QSharedPointer<JsonDefinition> def = builder.definition();

    QJsonObject json;
    JsonValidator validator(def);
    EXPECT_TRUE(validator.validate(json)) << validator.errors();

    // -- exists
    json["valid"] = "value";

    validator.useDefinition(def);
    EXPECT_TRUE(validator.validate(json)) << validator.errors();

    // -- not string type;
    json["valid"] = 1;
    EXPECT_FALSE(validator.validate(json)) << validator.errors();
    EXPECT_EQ(validator.errors().length(), 1);
}

TEST(JsonValidator, BooleanMember)
{
    JsonDefinitionBuilder builder;
    builder.hasBooleanMember("enabled");
    builder.hasOptionalBooleanMember("disabled");
    builder.hasOptionalBooleanMember("debugmode"); // this is not found
    QSharedPointer<JsonDefinition> def = builder.definition(); // using pointer to delete def in the test end

    QJsonObject json;
    json["enabled"] = true;
    json["disabled"] = false;

    JsonValidator validator(def);
    EXPECT_TRUE(validator.validate(json)) << validator.errors();
    EXPECT_EQ(validator.errors().length(), 0);

    // --

    json["enabled"] = 1;
    EXPECT_FALSE(validator.validate(json)) << validator.errors();
    EXPECT_EQ(validator.errors().length(), 1);
}
