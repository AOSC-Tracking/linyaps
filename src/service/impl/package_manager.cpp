/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "package_manager.h"

#include <signal.h>
#include <sys/types.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QThread>
#include <QProcess>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSysInfo>

#include "module/runtime/app.h"
#include "module/util/app_status.h"
#include "module/util/appinfo_cache.h"
#include "module/util/fs.h"
#include "module/util/sysinfo.h"
#include "module/package/info.h"
#include "module/repo/repo.h"
#include "dbus_retcode.h"
#include "job_manager.h"

using linglong::util::fileExists;
using linglong::util::listDirFolders;
using linglong::dbus::RetCode;

using linglong::service::util::AppInstance;
using linglong::service::util::AppInfo;

using namespace linglong;

class PackageManagerPrivate
{
public:
    explicit PackageManagerPrivate(PackageManager *parent)
        : q_ptr(parent)
    {
    }

    QMap<QString, QPointer<App>> apps;

    PackageManager *q_ptr = nullptr;
};

PackageManager::PackageManager()
    : dd_ptr(new PackageManagerPrivate(this))
    , app_instance_list(linglong::service::util::AppInstance::get())
{
    AppInfo app_info;
    app_info.appid = "org.test.app1";
    app_info.version = "v0.1";
    this->app_instance_list->AppendAppInstance<AppInfo>(app_info);

    // 检查安装数据库信息
    checkInstalledAppDb();
    updateInstalledAppInfoDb();

    // 检查应用缓存信息
    checkAppCache();
}

PackageManager::~PackageManager() = default;

/*!
 * 下载软件包
 * @param packageIDList
 */
RetMessageList PackageManager::Download(const QStringList &packageIDList, const QString savePath)
{
    // Q_D(PackageManager);

    // return JobManager::instance()->CreateJob([](Job *jr) {
    //     在这里写入真正的实现
    //     QProcess p;
    //     p.setProgram("curl");
    //     p.setArguments({"https://www.baidu.com"});
    //     p.start();
    //     p.waitForStarted();
    //     p.waitForFinished(-1);
    //     qDebug() << p.readAllStandardOutput();
    //     qDebug() << "finish" << p.exitStatus() << p.state();
    // });
    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    QString pkgName = packageIDList.at(0);
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qInfo() << "package name err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("package name err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    PackageManagerProxyBase *pImpl = PackageManagerImpl::instance();
    return pImpl->Download(packageIDList, savePath);
}

/*!
 * 在线安装软件包
 * @param packageIDList
 */
RetMessageList PackageManager::Install(const QStringList &packageIDList, const ParamStringMap &paramMap)
{
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_REPO_POINT)) {
        return PackageManagerFlatpakImpl::instance()->Install(packageIDList);
    }
    // Q_D(PackageManager);

    // return JobManager::instance()->CreateJob([](Job *jr) {
    //     // 在这里写入真正的实现
    //     QProcess p;
    //     p.setProgram("curl");
    //     p.setArguments({"https://www.baidu.com"});
    //     p.start();
    //     p.waitForStarted();
    //     p.waitForFinished(-1);
    //     qDebug() << p.readAllStandardOutput();
    //     qDebug() << "finish" << p.exitStatus() << p.state();
    // });
    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    QString pkgName = packageIDList.at(0);
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qInfo() << "package name err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("package name err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    PackageManagerProxyBase *pImpl = PackageManagerImpl::instance();
    return pImpl->Install(packageIDList, paramMap);
}

RetMessageList PackageManager::Uninstall(const QStringList &packageIDList, const ParamStringMap &paramMap)
{
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_REPO_POINT)) {
        return PackageManagerFlatpakImpl::instance()->Uninstall(packageIDList);
    }
    // 校验包名参数
    // 判断软件包是否安装
    // 卸载
    // 更新安装数据库
    // 更新本地软件包目录
    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    if (packageIDList.size() == 0) {
        qInfo() << "packageIDList input err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("packageIDList input err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    QString pkgName = packageIDList.at(0);
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qInfo() << "package name err";
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage("package name err");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    return PackageManagerImpl::instance()->Uninstall(packageIDList, paramMap);
}

QString PackageManager::Update(const QStringList &packageIDList)
{
    sendErrorReply(QDBusError::NotSupported, message().member());
    return {};
}

QString PackageManager::UpdateAll()
{
    sendErrorReply(QDBusError::NotSupported, message().member());
    return {};
}

/*
 * 查询软件包
 *
 * @param packageIDList: 软件包的appid
 *
 * @return AppMetaInfoList 查询结果列表
 */
AppMetaInfoList PackageManager::Query(const QStringList &packageIDList, const ParamStringMap &paramMap)
{
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_REPO_POINT)) {
        return PackageManagerFlatpakImpl::instance()->Query(packageIDList);
    }
    QString pkgName = packageIDList.at(0);
    if (pkgName.isNull() || pkgName.isEmpty()) {
        qInfo() << "package name err";
        return {};
    }
    PackageManagerProxyBase *pImpl = PackageManagerImpl::instance();
    return pImpl->Query(packageIDList, paramMap);
}

/*!
 * 安装本地软件包
 * @param packagePathList
 */
QString PackageManager::Import(const QStringList &packagePathList)
{
    sendErrorReply(QDBusError::NotSupported, message().member());
    return {};
}

QString appConfigPath(const QString &appID, const QString &appVersion, bool isFlatpakApp = false)
{
    util::ensureUserDir({".linglong", appID});

    auto configPath = getUserFile(QString("%1/%2/app.yaml").arg(".linglong", appID));

    if (QFile::exists(configPath)) {
        return configPath;
    }
    // create yaml form info
    // auto appRoot = LocalRepo::get()->rootOfLatest();
    auto latestAppRef = repo::latestOf(appID, appVersion);

    auto appInstallRoot = repo::rootOfLayer(latestAppRef);

    auto appInfo = appInstallRoot + "/info.json";
    // 判断是否存在
    if (!isFlatpakApp && !fileExists(appInfo)) {
        qCritical() << appInfo << " not exist";
        return "";
    }

    // create a yaml config from json
    auto info = util::loadJSON<package::Info>(appInfo);

    if (info->runtime.isEmpty()) {
        // FIXME: return error is not exist

        // thin runtime
        info->runtime = "org.deepin.Runtime/20/x86_64";

        // full runtime
        // info->runtime = "deepin.Runtime.Sdk/23/x86_64";
    }

    package::Ref runtimeRef(info->runtime);

    QMap<QString, QString> variables = {
        {"APP_REF", latestAppRef.toLocalRefString()},
        {"RUNTIME_REF", runtimeRef.toLocalRefString()},
    };

    // TODO: remove to util module as file_template.cpp

    QFile templateFile(":/app.yaml");
    templateFile.open(QIODevice::ReadOnly);
    auto templateData = templateFile.readAll();
    foreach (auto const &k, variables.keys()) {
        templateData.replace(QString("@%1@").arg(k).toLocal8Bit(), variables.value(k).toLocal8Bit());
    }
    templateFile.close();

    QFile configFile(configPath);
    configFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    configFile.write(templateData);
    configFile.close();

    return configPath;
}

/*
 * 执行软件包
 *
 * @param packageID: 软件包的appid
 * @param paramMap: 运行参数信息
 *
 * @return RetMessageList: 运行结果信息
 */
RetMessageList PackageManager::Start(const QString &packageID, const ParamStringMap &paramMap)
{
    Q_D(PackageManager);

    qDebug() << "start package" << packageID;

    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);

    // 获取版本信息
    QString version = "";
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_VERSION)) {
        version = paramMap[linglong::util::KEY_VERSION];
    }

    //获取exec参数
    QString desktopExec;
    desktopExec.clear();
    if (!paramMap.empty() && paramMap.contains(linglong::util::KEY_EXEC)) {
        desktopExec = paramMap[linglong::util::KEY_EXEC];
    }

    // 判断是否已安装
    QString err = "";
    QString userName = getUserName();
    if (!getAppInstalledStatus(packageID, version, "", userName)) {
        err = packageID + " not installed";
        qCritical() << err;
        info->setcode(RetCode(RetCode::pkg_not_installed));
        info->setmessage(err);
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    JobManager::instance()->CreateJob([=](Job *jr) {
        // 判断是否存在
        bool isFlatpakApp = !paramMap.empty() && paramMap.contains(linglong::util::KEY_REPO_POINT);
        QString configPath = appConfigPath(packageID, version, isFlatpakApp);
        if (!fileExists(configPath)) {
            return;
        }
        auto app = App::load(configPath, desktopExec, isFlatpakApp);
        if (nullptr == app) {
            // FIXME: set job status to failed
            qCritical() << "nullptr" << app;
            return;
        }
        d->apps[app->container()->ID] = QPointer<App>(app);
        app->start();
    });
    return retMsg;
}

/*
 * 停止应用
 *
 * @param containerID: 应用启动对应的容器ID
 *
 * @return RetMessageList 执行结果信息
 */
RetMessageList PackageManager::Stop(const QString &containerID)
{
    Q_D(PackageManager);

    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    QString err = "";
    if (!d->apps.contains(containerID)) {
        err = "containerID:" + containerID + " not exist";
        qCritical() << err;
        info->setcode(RetCode(RetCode::user_input_param_err));
        info->setmessage(err);
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    pid_t pid = d->apps[containerID]->container()->PID;
    int ret = kill(pid, SIGKILL);
    if (ret == 0) {
        d->apps.remove(containerID);
    } else {
        err = "kill container failed, containerID:" + containerID;
        info->setcode(RetCode(RetCode::ErrorPkgKillFailed));
        info->setmessage(err);
        info->setstate(false);
        retMsg.push_back(info);
    }
    qInfo() << "kill containerID:" << containerID << ",ret:" << ret;
    return retMsg;
}

ContainerList PackageManager::ListContainer()
{
    Q_D(PackageManager);
    ContainerList list;

    for (const auto &app : d->apps) {
        auto c = QPointer<Container>(new Container);
        c->ID = app->container()->ID;
        c->PID = app->container()->PID;
        list.push_back(c);
    }
    return list;
}
QString PackageManager::Status()
{
    return {"active"};
}
