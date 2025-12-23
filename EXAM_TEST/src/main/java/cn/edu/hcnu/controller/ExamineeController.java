package cn.edu.hcnu.controller;

import cn.edu.hcnu.po.Examinee;
import cn.edu.hcnu.service.ExamineeService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestBody;

@Controller

public class ExamineeController {
    @Autowired
    private ExamineeService examineeService;

 @RequestBody