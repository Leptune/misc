<?php

use Meiya\Exception\CreateVideoConvertTaskFailException;

class QiniuVideoConvertTask extends QiniuBase
{
    /* 获取管理凭证
     * @param  string $requestUrl
     * @param  array  $body 请求的参数数组
     * @return string 管理凭证
     */
    private function getAccessToken($requestUrl, $body)
    {
        $mac = \Qiniu_RequireMac(null);

        $request = new \Qiniu_Request(['path' => $requestUrl], $body);
        $token = $mac->SignRequest($request, true);

        return $token;
    }

    /* 由给定后缀生成要保存的key
     * @param     string $key
     * @param     string $suffix
     * @return    string 返回生成的要保存的key
     * @exception CreateVideoConvertTaskFailException
     */
    private function makeSaveName($key, $suffix)
    {
        // 去除原先的后缀
        $keyArr = explode('.', $key);
        // 只有key原先有后缀，才需要先去除后缀
        if (count($keyArr) >= 2) {
            array_pop($keyArr);
        }

        // 加上要生成的后缀名
        $keyArr[count($keyArr) - 1] .= $suffix;
        $saveKey = implode('.', $keyArr);
        if ($saveKey === $key) {
            throw new CreateVideoConvertTaskFailException('要保存的名字不能和源资源名一样');
        }

        // 指定要存储的空间名
        $saveName = $this->bucket . ':' . $saveKey;
        $saveName = UrlHelper::urlSafeBase64Encode($saveName);

        return $saveName;
    }

    /* 由给定的url生成七牛所需要的key
     * @param     string $url
     * @return    string key
     * @exception CreateVideoConvertTaskFailException
     */
    private function makeKey($url)
    {
        $urlParse = parse_url($url);
        if (false === $urlParse) {
            throw new CreateVideoConvertTaskFailException('url不合法');
        }
        // 去除path的第一个字符'/'
        $key = substr($urlParse['path'], 1);

        return $key;
    }

    /* 根据提供的fops数组生成七牛所需要的fops字符串格式
     * @param  array $fops
     * @return string
     */
    public static function makeFops($fops)
    {
        $data = [];
        foreach ($fops as $key => $value) {
            $data []= "$key/$value";
        }

        return implode('/', $data);
    }

    /* 生成请求触发持久化处理(pfop)的options
     * @param  string       $requestUrl: 请求七牛的url
     * @param  string       $url
     * @param  string|array $fops
     * @param  string       $suffix
     * @return array        options数组
     */
    private function makeRequestOptions($requestUrl, $url, $fops, $suffix = NULL)
    {
        if (is_array($fops)) {
            $fops = $this->makeFops($fops);
        }
        $key       = $this->makeKey($url);
        $pipeline  = Config::get('services.qiniu.video_pipeline');
        if (!is_null($suffix)) {
            $saveName = $this->makeSaveName($key, $suffix);
            $fops .= "|saveas/$saveName";
        }

        $body = [
            'bucket'    => $this->bucket,
            'key'       => $key,
            'fops'      => $fops,
            'notifyURL' => $this->notifyUrl,
            'pipeline'  => $pipeline,
        ];

        $body = http_build_query($body);
        $acessToken = $this->getAccessToken($requestUrl, $body);
        $header = "Host: api.qiniu.com\r\n" .
                  "Content-Type: application/x-www-form-urlencoded\r\n" .
                  "Authorization: QBox $acessToken\r\n";
        $options = [
            'http' => [
                'header'  => $header,
                'method'  => 'POST',
                'content' => $body,
            ],
        ];

        return $options;
    }

    /* 封装触发持久化处理(pfop)接口
     * @param  string $url        : 源资源的url
     * @param  string|array $fops : 云处理操作列表，含义请参见七牛的persistentOps详解文档
     * @param  string $suffix     : 保存的名字为替换原先url的后缀
     *                             如$suffix为'-320x240.mp4'则保存的名字为：
     *                             http://leptune-asdf.mp4 => http://leptune-asdf-320x240.mp4
     * @return string             : 任务id
     * @exception CreateVideoConvertTaskFailException
     */
    public function pfop($url, $fops, $suffix = NULL)
    {
        $requestUrl = 'http://api.qiniu.com/pfop/';
        $options    = $this->makeRequestOptions($requestUrl, $url, $fops, $suffix);
        $context    = stream_context_create($options);
        $result     = file_get_contents($requestUrl, false, $context);
        if (!empty($request)) {
            throw new CreateVideoConvertTaskFailException('创建七牛转换视频任务失败');
        }

        $result = json_decode($result);
        if (isset($result->error)) {
            throw new CreateVideoConvertTaskFailException('创建七牛转换视频任务失败');
        }
        return $result->persistentId;
    }
}
