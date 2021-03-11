#pragma once
#include <string>
#include <vector>
#include <map>

const std::string strDetectAlgTypeIdxValue("0");
const std::string strActionAlgTypeIdxValue("1");
const std::string strFaceAlgTypeIdxValue("2");

static std::map<string, string> AlgCfgMap =
{
	{std::make_pair(strDetectAlgTypeIdxValue,"detect1")},
	{std::make_pair(strActionAlgTypeIdxValue,"action1")},
	{std::make_pair(strFaceAlgTypeIdxValue,"face1")}
};

static std::map<string, string> DetectLabsMap =
{
	{std::make_pair("0","人")},{std::make_pair("1","自行车")},{std::make_pair("2","小车")},{std::make_pair("3","摩托车")},{std::make_pair("4","巴士")},
	{std::make_pair("5","火车")},{std::make_pair("6","卡车")},{std::make_pair("7","船")},{std::make_pair("8","交通灯")},{std::make_pair("9","消防栓")},
	{std::make_pair("10","停止标志")},{std::make_pair("11","猫")},{std::make_pair("12","狗")},{std::make_pair("13","背包")},{std::make_pair("14","雨伞")},
    {std::make_pair("15","手提包")},{std::make_pair("16","手提箱")},{std::make_pair("17","刀")},{std::make_pair("18","笔记本电脑")},{std::make_pair("19","手机")},
	{std::make_pair("20","火")},{std::make_pair("21","电动车")},{std::make_pair("22","工程车")},{std::make_pair("23","头盔")},{std::make_pair("24","香烟")},
	{std::make_pair("25","烟雾")}
};

static std::map<string,string> ActionLabsMap =
{
    {std::make_pair("0","抽烟")},{std::make_pair("1","闯入")},{std::make_pair("2","摔倒")},{std::make_pair("3","徘徊")},{std::make_pair("4","攀爬")},{std::make_pair("5","打架")}
};