/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QDBusArgument>
#include <QDBusContext>
#include <QList>
#include <QObject>
#include <QScopedPointer>

#include <DSingleton>

#include "module/package/package.h"
#include "json_register_inc.h"
#include "module/runtime/container.h"
#include "module/package/pkginfo.h"
#include "qdbus_retmsg.h"
#include "../util/singleton.h"

using linglong::service::util::AppInstance;
// 当前OUAP在线包对应的包名/版本/架构/存储URL信息 to do fix
typedef struct _AppStreamPkgInfo {
    QString appId;
    QString appName;
    QString appVer;
    QString appArch;
    QString appUrl;

    QString summary;
    QString runtime;
    QString reponame;

} AppStreamPkgInfo;

class PackageManagerPrivate;
class PackageManager : public QObject
    , protected QDBusContext
    , public Dtk::Core::DSingleton<PackageManager>
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.linglong.PackageManager")

    friend class Dtk::Core::DSingleton<PackageManager>;

public Q_SLOTS:
    RetMessageList Download(const QStringList &packageIDList, const QString savePath);
    RetMessageList Install(const QStringList &packageIDList);
    QString Uninstall(const QStringList &packageIDList);
    QString Update(const QStringList &packageIDList);
    QString UpdateAll();

    PKGInfoList Query(const QStringList &packageIDList);

    QString Import(const QStringList &packagePathList);

    QString Start(const QString &packageID);
    void Stop(const QString &containerID);
    ContainerList ListContainer();

    /*!
     * QDbusRetInfo
     * @param packageIDList appid
     * @return PKGInfoList
     */
    PKGInfoList QDbusRetInfo(const QStringList &packageIDList)
    {
        PKGInfoList list;
        qInfo() << "recv: " + QString::number(packageIDList.size());
        for (const auto &it : packageIDList) {
            qInfo() << "appid:" << it;
        }
        for (int i = 0; i < 3; i++) {
            auto c = QPointer<PKGInfo>(new PKGInfo);
            c->appid = "org.deepin.test-" + QString::number(i);
            c->appname = "test-" + QString::number(i);
            c->version = "v" + QString::number(i);
            list.push_back(c);
        }
        return list;
    };

    /*!
     * QDbusMessageRet
     * @return RetMessageList
     */
    RetMessageList QDbusMessageRet(void)
    {
        RetMessageList list;

        qInfo() << "call: QDbusMessageRet";

        for (int i = 0; i < 3; i++) {
            auto c = QPointer<RetMessage>(new RetMessage);
            c->state = false;
            c->code = 404;
            c->message = "not found!";
            list.push_back(c);
        }

        return list;
    };

protected:
    AppInstance *app_instance_list ;

protected:
    PackageManager();
    ~PackageManager() override;

private:
    QScopedPointer<PackageManagerPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), PackageManager)

    AppStreamPkgInfo appStreamPkgInfo;

    /*
     * 根据AppStream.json查询目标软件包信息
     *
     * @param savePath: AppStream.json文件存储路径
     * @param remoteName: 远端仓库名称
     * @param pkgName: 软件包包名
     * @param pkgArch: 软件包对应的架构
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool getAppInfoByAppStream(const QString savePath, const QString remoteName, const QString pkgName, const QString pkgArch, QString &err);

    /*
     * 更新本地AppStream.json文件
     *
     * @param savePath: AppStream.json文件存储路径
     * @param remoteName: 远端仓库名称
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool updateAppStream(const QString savePath, const QString remoteName, QString &err);

    /*
     * 通过AppStream.json更新OUAP在线包
     *
     * @param xmlPath: AppStream.json文件存储路径
     * @param savePath: OUAP在线包存储路径
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool updateOUAP(const QString xmlPath, const QString savePath, QString &err);

    /*
     * 解析OUAP在线包中的info.json配置信息
     *
     * @param infoPath: info.json文件存储路径
     *
     * @return bool: true:成功 false:失败
     */
    bool resolveOUAPCfg(const QString infoPath);

    /*
     * 将OUAP在线包数据部分签出到指定目录
     *
     * @param pkgName: 软件包包名
     * @param pkgArch: 软件包对应的架构
     * @param dstPath: OUAP在线包数据部分存储路径
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool downloadOUAPData(const QString pkgName, const QString pkgArch, const QString dstPath, QString &err);

    /*
     * 将OUAP在线包解压到指定目录
     *
     * @param ouapPath: ouap在线包存储路径
     * @param savePath: 解压后文件存储路径
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool extractOUAP(const QString ouapPath, const QString savePath, QString &err);

    /*
     * 根据OUAP在线包数据生成对应的离线包
     *
     * @param cfgPath: OUAP在线包数据存储路径
     * @param dstPath: 生成离线包存储路径
     *
     * @return bool: true:成功 false:失败
     */
    bool makeUAPbyOUAP(QString cfgPath, QString dstPath);

    /*
     * 更新应用安装状态到本地文件
     *
     * @param appStreamPkgInfo: 安装成功的软件包信息
     *
     * @return bool: true:成功 false:失败
     */
    bool updateAppStatus(AppStreamPkgInfo appStreamPkgInfo);

    /*
     * 查询软件包安装状态
     *
     * @param pkgName: 软件包包名
     * @param userName: 用户名，默认为当前用户
     *
     * @return bool: 1:已安装 0:未安装 -1查询失败
     */
    int getIntallStatus(const QString pkgName, const QString userName = "");

    /*
     * 查询已安装软件包信息
     *
     * @param pkgName: 软件包包名
     * @param pkgList: 查询结果
     *
     * @return bool: true:成功 false:失败(软件包未安装)
     */
    bool getInstalledAppInfo(const QString pkgName, PKGInfoList &pkgList);

    /*
     * 查询未安装软件包信息
     *
     * @param pkgName: 软件包包名
     * @param pkgList: 查询结果
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool getUnInstalledAppInfo(const QString pkgName, PKGInfoList &pkgList, QString &err);

    /*
     * 建立box运行应用需要的软链接
     */
    void buildRequestedLink();

    /*
     * 根据OUAP在线包解压出来的uap文件查询软件包信息
     *
     * @param fileDir: OUAP在线包文件解压后的uap文件存储路径
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool getAppInfoByOUAPFile(const QString fileDir, QString& err);

    /*
     * 根据OUAP在线包文件安装软件包
     *
     * @param filePath: OUAP在线包文件路径
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool installOUAPFile(const QString filePath, QString &err);

    /*
     * 查询系统架构
     *
     * @return QString: 系统架构字符串
     */
    const QString getHostArch();

    /*
     * 查询当前登陆用户名
     *
     * @return QString: 当前登陆用户名
     */
    const QString getUserName();
};