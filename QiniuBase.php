<?php

use Meiya\Exception\CreateVideoConvertTaskFailException;

class QiniuBase
{
    protected $bucket;
    protected $accessKey;
    protected $secretKey;

    public function __construct()
    {
        $this->bucket    = Config::get('services.qiniu.bucket');
        $this->accessKey = Config::get('services.qiniu.access_key');
        $this->secretKey = Config::get('services.qiniu.secret_key');
        $this->notifyUrl = Config::get('services.qiniu.persistent_notify_url');
        \Qiniu_setKeys($this->accessKey, $this->secretKey);
    }

    /* 验证是否是七牛的回调
     */
    public function isQiniuCallback()
    {
        // @TODO 由于七牛的回调里的_SERVER数组里既没有HTTP_AUTHORIZATION，也没有AUTHORIZATION。所以暂不校验。后续待调试后再开启。（查看七牛的回调安全性链接以获得更详细信息：http://developer.qiniu.com/docs/v6/api/overview/up/response/callback.html）
        return;

        if (isset($_SERVER['HTTP_AUTHORIZATION'])) {
            $authstr = $_SERVER['HTTP_AUTHORIZATION'];
        } else if (isset($_SERVER['AUTHORIZATION'])) {
            $authstr = $_SERVER['AUTHORIZATION'];
        } else {
            throw new CreateVideoConvertTaskFailException('unset _SERVER(HTTP_AUTHORIZATION) and _SERVER(AUTHORIZATION)');
        }

        $authPrefix = 'QBox ';
        if (strpos($authstr, $authPrefix) != 0) {
            throw new CreateVideoConvertTaskFailException('not begin with "QBox "');
        }

        $auth = explode(":", substr($authstr, strlen($authPrefix)));
        if (sizeof($auth) != 2) {
            throw new CreateVideoConvertTaskFailException('Qiniu AUTHORIZATION array sepereated by ":" not have 2 item');
        }
        if ($auth[0] != $this->accessKey) {
            throw new CreateVideoConvertTaskFailException('accessKey not right');
        }

        $urlParse  = parse_url($this->notifyUrl);
        $path      = $urlParse['path'];

        $inputData = [];
        foreach (Request::input() as $key => $value) {
            $inputData []= $key . '=' . $value;
        }
        $inputData = urlencode(implode('&', $inputData));

        $data     = "$path\n" . $inputData;
        $signData = UrlHelper::urlSafeBase64Encode(hash_hmac('sha1', $data, $this->secretKey, true));

        if ($signData != $auth[1]) {
            throw new CreateVideoConvertTaskFailException('signData not right');
        }

        return;
    }
}
