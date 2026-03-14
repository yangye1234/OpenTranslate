#include "l10n.h"

namespace {
QString en(const QString &key)
{
    if (key == "dialog.title") return "OpenTranslate";
    if (key == "dialog.original.placeholder") return "Enter source text and press Enter";
    if (key == "dialog.result.placeholder") return "Translation result";
    if (key == "dialog.error.invalid_pair") return "Invalid language pair.";
    if (key == "dialog.status.translating") return "Translating...";
    if (key == "dialog.tooltip.swap") return "Swap language direction";
    if (key == "dialog.tooltip.pin") return "Pin on top";
    if (key == "dialog.tooltip.settings") return "Open settings";

    if (key == "settings.title") return "Settings";
    if (key == "settings.group.app") return "Application";
    if (key == "settings.label.app_language") return "App Language";
    if (key == "settings.group.baidu") return "Baidu API";
    if (key == "settings.baidu.enabled") return "Enable Baidu translator";
    if (key == "settings.baidu.app_id") return "App ID";
    if (key == "settings.baidu.app_key") return "App Key";
    if (key == "settings.group.generic") return "Generic LLM API (V1 stores only)";
    if (key == "settings.generic.enabled") return "Enable Generic API (not called in V1)";
    if (key == "settings.generic.base_url") return "Base URL";
    if (key == "settings.generic.model") return "Model";
    if (key == "settings.generic.api_key") return "API Key";
    if (key == "settings.generic.prompt") return "Prompt Template";
    if (key == "settings.group.pairs") return "Language Pairs";
    if (key == "settings.pairs.placeholder") return "en->zh";
    if (key == "settings.pairs.add") return "Add";
    if (key == "settings.pairs.edit") return "Edit Selected";
    if (key == "settings.pairs.remove") return "Remove";
    if (key == "settings.save") return "Save";

    if (key == "settings.error.invalid_pair.title") return "Invalid language pair";
    if (key == "settings.error.invalid_pair.body") return "Use format like en->zh.";
    if (key == "settings.error.duplicate.title") return "Duplicate";
    if (key == "settings.error.duplicate.body") return "This language pair already exists.";

    if (key == "language.english") return "English";
    if (key == "language.zh_cn") return "简体中文";
    if (key == "language.zh_tw") return "繁體中文";

    return key;
}

QString zhCN(const QString &key)
{
    if (key == "dialog.title") return "开源翻译";
    if (key == "dialog.original.placeholder") return "输入原文后按回车翻译";
    if (key == "dialog.result.placeholder") return "翻译结果";
    if (key == "dialog.error.invalid_pair") return "语言方向格式不正确。";
    if (key == "dialog.status.translating") return "翻译中...";
    if (key == "dialog.tooltip.swap") return "切换翻译方向";
    if (key == "dialog.tooltip.pin") return "窗口置顶";
    if (key == "dialog.tooltip.settings") return "打开设置";

    if (key == "settings.title") return "设置";
    if (key == "settings.group.app") return "应用";
    if (key == "settings.label.app_language") return "应用语言";
    if (key == "settings.group.baidu") return "百度翻译 API";
    if (key == "settings.baidu.enabled") return "启用百度翻译";
    if (key == "settings.baidu.app_id") return "App ID";
    if (key == "settings.baidu.app_key") return "App Key";
    if (key == "settings.group.generic") return "通用大模型 API（V1 仅保存配置）";
    if (key == "settings.generic.enabled") return "启用通用 API（V1 不调用）";
    if (key == "settings.generic.base_url") return "Base URL";
    if (key == "settings.generic.model") return "模型";
    if (key == "settings.generic.api_key") return "API Key";
    if (key == "settings.generic.prompt") return "翻译提示词";
    if (key == "settings.group.pairs") return "语言方向";
    if (key == "settings.pairs.placeholder") return "例如 en->zh";
    if (key == "settings.pairs.add") return "新增";
    if (key == "settings.pairs.edit") return "编辑当前";
    if (key == "settings.pairs.remove") return "删除";
    if (key == "settings.save") return "保存";

    if (key == "settings.error.invalid_pair.title") return "语言方向格式错误";
    if (key == "settings.error.invalid_pair.body") return "请使用类似 en->zh 的格式。";
    if (key == "settings.error.duplicate.title") return "重复";
    if (key == "settings.error.duplicate.body") return "该语言方向已存在。";

    if (key == "language.english") return "English";
    if (key == "language.zh_cn") return "简体中文";
    if (key == "language.zh_tw") return "繁體中文";

    return key;
}

QString zhTW(const QString &key)
{
    if (key == "dialog.title") return "開源翻譯";
    if (key == "dialog.original.placeholder") return "輸入原文後按 Enter 翻譯";
    if (key == "dialog.result.placeholder") return "翻譯結果";
    if (key == "dialog.error.invalid_pair") return "語言方向格式不正確。";
    if (key == "dialog.status.translating") return "翻譯中...";
    if (key == "dialog.tooltip.swap") return "切換翻譯方向";
    if (key == "dialog.tooltip.pin") return "視窗置頂";
    if (key == "dialog.tooltip.settings") return "開啟設定";

    if (key == "settings.title") return "設定";
    if (key == "settings.group.app") return "應用";
    if (key == "settings.label.app_language") return "應用語言";
    if (key == "settings.group.baidu") return "百度翻譯 API";
    if (key == "settings.baidu.enabled") return "啟用百度翻譯";
    if (key == "settings.baidu.app_id") return "App ID";
    if (key == "settings.baidu.app_key") return "App Key";
    if (key == "settings.group.generic") return "通用大模型 API（V1 僅保存設定）";
    if (key == "settings.generic.enabled") return "啟用通用 API（V1 不呼叫）";
    if (key == "settings.generic.base_url") return "Base URL";
    if (key == "settings.generic.model") return "模型";
    if (key == "settings.generic.api_key") return "API Key";
    if (key == "settings.generic.prompt") return "翻譯提示詞";
    if (key == "settings.group.pairs") return "語言方向";
    if (key == "settings.pairs.placeholder") return "例如 en->zh";
    if (key == "settings.pairs.add") return "新增";
    if (key == "settings.pairs.edit") return "編輯目前";
    if (key == "settings.pairs.remove") return "刪除";
    if (key == "settings.save") return "保存";

    if (key == "settings.error.invalid_pair.title") return "語言方向格式錯誤";
    if (key == "settings.error.invalid_pair.body") return "請使用類似 en->zh 的格式。";
    if (key == "settings.error.duplicate.title") return "重複";
    if (key == "settings.error.duplicate.body") return "該語言方向已存在。";

    if (key == "language.english") return "English";
    if (key == "language.zh_cn") return "简体中文";
    if (key == "language.zh_tw") return "繁體中文";

    return key;
}
}

namespace L10n {

QString text(AppLanguage language, const QString &key)
{
    switch (language) {
    case AppLanguage::English:
        return en(key);
    case AppLanguage::TraditionalChinese:
        return zhTW(key);
    case AppLanguage::SimplifiedChinese:
    default:
        return zhCN(key);
    }
}

} // namespace L10n
