package cn.edu.hcnu.controller;

import cn.edu.hcnu.po.Examinee;
import cn.edu.hcnu.service.ExamineeService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;

@Controller

public class ExamineeController {
    @Autowired
    private ExamineeService examineeService;

    @GetMapping("/getAllExaminee")
@ResponseBody
public List<Examinee> get
}
